/**
 * Copyright © 2019 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "host_notifier.hpp"

#include <phosphor-logging/log.hpp>

namespace openpower::pels
{

const auto subscriptionName = "PELHostNotifier";
const size_t maxRetryAttempts = 15;

using namespace phosphor::logging;

HostNotifier::HostNotifier(Repository& repo, DataInterfaceBase& dataIface,
                           std::unique_ptr<HostInterface> hostIface) :
    _repo(repo),
    _dataIface(dataIface), _hostIface(std::move(hostIface)),
    _retryTimer(_hostIface->getEvent(),
                std::bind(std::mem_fn(&HostNotifier::retryTimerExpired), this)),
    _hostFullTimer(
        _hostIface->getEvent(),
        std::bind(std::mem_fn(&HostNotifier::hostFullTimerExpired), this))
{
    // Subscribe to be told about new PELs.
    _repo.subscribeToAdds(subscriptionName,
                          std::bind(std::mem_fn(&HostNotifier::newLogCallback),
                                    this, std::placeholders::_1));

    // Subscribe to be told about deleted PELs.
    _repo.subscribeToDeletes(
        subscriptionName,
        std::bind(std::mem_fn(&HostNotifier::deleteLogCallback), this,
                  std::placeholders::_1));

    // Add any existing PELs to the queue to send them if necessary.
    _repo.for_each(std::bind(std::mem_fn(&HostNotifier::addPELToQueue), this,
                             std::placeholders::_1));

    // Subscribe to be told about host state changes.
    _dataIface.subscribeToHostStateChange(
        subscriptionName,
        std::bind(std::mem_fun(&HostNotifier::hostStateChange), this,
                  std::placeholders::_1));

    // Set the function to call when the async reponse is received.
    _hostIface->setResponseFunction(
        std::bind(std::mem_fn(&HostNotifier::commandResponse), this,
                  std::placeholders::_1));

    // Start sending logs if the host is running
    if (!_pelQueue.empty() && _dataIface.isHostUp())
    {
        doNewLogNotify();
    }
}

HostNotifier::~HostNotifier()
{
    _repo.unsubscribeFromAdds(subscriptionName);
    _dataIface.unsubscribeFromHostStateChange(subscriptionName);
}

bool HostNotifier::addPELToQueue(const PEL& pel)
{
    if (enqueueRequired(pel.id()))
    {
        _pelQueue.push_back(pel.id());
    }

    // Return false so that Repo::for_each keeps going.
    return false;
}

bool HostNotifier::enqueueRequired(uint32_t id) const
{
    bool required = true;
    Repository::LogID i{Repository::LogID::Pel{id}};

    // Manufacturing testing may turn off sending up PELs
    if (!_dataIface.getHostPELEnablement())
    {
        return false;
    }

    if (auto attributes = _repo.getPELAttributes(i); attributes)
    {
        auto a = attributes.value().get();

        if ((a.hostState == TransmissionState::acked) ||
            (a.hostState == TransmissionState::badPEL))
        {
            required = false;
        }
        else if (a.actionFlags.test(hiddenFlagBit) &&
                 (a.hmcState == TransmissionState::acked))
        {
            required = false;
        }
        else if (a.actionFlags.test(dontReportToHostFlagBit))
        {
            required = false;
        }
    }
    else
    {
        using namespace phosphor::logging;
        log<level::ERR>("Host Enqueue: Unable to find PEL ID in repository",
                        entry("PEL_ID=0x%X", id));
        required = false;
    }

    return required;
}

bool HostNotifier::notifyRequired(uint32_t id) const
{
    bool notify = true;
    Repository::LogID i{Repository::LogID::Pel{id}};

    if (auto attributes = _repo.getPELAttributes(i); attributes)
    {
        // If already acked by the host, don't send again.
        // (A safety check as it shouldn't get to this point.)
        auto a = attributes.value().get();
        if (a.hostState == TransmissionState::acked)
        {
            notify = false;
        }
        else if (a.actionFlags.test(hiddenFlagBit))
        {
            // If hidden and acked (or will be) acked by the HMC,
            // also don't send it. (HMC management can come and
            // go at any time)
            if ((a.hmcState == TransmissionState::acked) ||
                _dataIface.isHMCManaged())
            {
                notify = false;
            }
        }
    }
    else
    {
        // Must have been deleted since put on the queue.
        notify = false;
    }

    return notify;
}

void HostNotifier::newLogCallback(const PEL& pel)
{
    if (!enqueueRequired(pel.id()))
    {
        return;
    }

    log<level::DEBUG>("new PEL added to queue", entry("PEL_ID=0x%X", pel.id()));

    _pelQueue.push_back(pel.id());

    // Notify shouldn't happen if host is down or full
    if (!_dataIface.isHostUp() || _hostFull)
    {
        return;
    }

    // Dispatch a command now if there isn't currently a command
    // in progress and this is the first log in the queue or it
    // previously gave up from a hard failure.
    auto inProgress = (_inProgressPEL != 0) || _hostIface->cmdInProgress() ||
                      _retryTimer.isEnabled();

    auto firstPEL = _pelQueue.size() == 1;
    auto gaveUp = _retryCount >= maxRetryAttempts;

    if (!inProgress && (firstPEL || gaveUp))
    {
        _retryCount = 0;

        // Send a log, but from the event loop, not from here.
        scheduleDispatch();
    }
}

void HostNotifier::deleteLogCallback(uint32_t id)
{
    auto queueIt = std::find(_pelQueue.begin(), _pelQueue.end(), id);
    if (queueIt != _pelQueue.end())
    {
        log<level::DEBUG>("Host notifier removing deleted log from queue");
        _pelQueue.erase(queueIt);
    }

    auto sentIt = std::find(_sentPELs.begin(), _sentPELs.end(), id);
    if (sentIt != _sentPELs.end())
    {
        log<level::DEBUG>("Host notifier removing deleted log from sent list");
        _sentPELs.erase(sentIt);
    }

    // Nothing we can do about this...
    if (id == _inProgressPEL)
    {
        log<level::WARNING>(
            "A PEL was deleted while its host notification was in progress",
            entry("PEL_ID=0x%X", id));
    }
}

void HostNotifier::scheduleDispatch()
{
    _dispatcher = std::make_unique<sdeventplus::source::Defer>(
        _hostIface->getEvent(), std::bind(std::mem_fn(&HostNotifier::dispatch),
                                          this, std::placeholders::_1));
}

void HostNotifier::dispatch(sdeventplus::source::EventBase& source)
{
    _dispatcher.reset();

    doNewLogNotify();
}

void HostNotifier::doNewLogNotify()
{
    if (!_dataIface.isHostUp() || _retryTimer.isEnabled() ||
        _hostFullTimer.isEnabled())
    {
        return;
    }

    if (_retryCount >= maxRetryAttempts)
    {
        // Give up until a new log comes in.
        if (_retryCount == maxRetryAttempts)
        {
            // If this were to really happen, the PLDM interface
            // would be down and isolating that shouldn't left to
            // a logging daemon, so just trace.  Also, this will start
            // trying again when the next new log comes in.
            log<level::ERR>(
                "PEL Host notifier hit max retry attempts. Giving up for now.",
                entry("PEL_ID=0x%X", _pelQueue.front()));

            // Tell the host interface object to clean itself up, especially to
            // release the PLDM instance ID it's been using.
            _hostIface->cancelCmd();
        }
        return;
    }

    bool doNotify = false;
    uint32_t id = 0;

    // Find the PEL to send
    while (!doNotify && !_pelQueue.empty())
    {
        id = _pelQueue.front();
        _pelQueue.pop_front();

        if (notifyRequired(id))
        {
            doNotify = true;
        }
    }

    if (doNotify)
    {
        // Get the size using the repo attributes
        Repository::LogID i{Repository::LogID::Pel{id}};
        if (auto attributes = _repo.getPELAttributes(i); attributes)
        {
            auto size = static_cast<size_t>(
                std::filesystem::file_size((*attributes).get().path));

            log<level::DEBUG>("sendNewLogCmd", entry("PEL_ID=0x%X", id),
                              entry("PEL_SIZE=%d", size));

            auto rc = _hostIface->sendNewLogCmd(id, size);

            if (rc == CmdStatus::success)
            {
                _inProgressPEL = id;
            }
            else
            {
                // It failed.  Retry
                log<level::ERR>("PLDM send failed", entry("PEL_ID=0x%X", id));
                _pelQueue.push_front(id);
                _inProgressPEL = 0;
                _retryTimer.restartOnce(_hostIface->getSendRetryDelay());
            }
        }
        else
        {
            log<level::ERR>("PEL ID not in repository.  Cannot notify host",
                            entry("PEL_ID=0x%X", id));
        }
    }
}

void HostNotifier::hostStateChange(bool hostUp)
{
    _retryCount = 0;
    _hostFull = false;

    if (hostUp && !_pelQueue.empty())
    {
        log<level::DEBUG>("Host state change to on");

        doNewLogNotify();
    }
    else if (!hostUp)
    {
        log<level::DEBUG>("Host state change to off");

        stopCommand();

        // Reset the state on any PELs that were sent but not acked back
        // to new so they'll get sent again.
        for (auto id : _sentPELs)
        {
            _pelQueue.push_back(id);
            _repo.setPELHostTransState(id, TransmissionState::newPEL);
        }

        _sentPELs.clear();

        if (_hostFullTimer.isEnabled())
        {
            _hostFullTimer.setEnabled(false);
        }
    }
}

void HostNotifier::commandResponse(ResponseStatus status)
{
    auto id = _inProgressPEL;
    _inProgressPEL = 0;

    if (status == ResponseStatus::success)
    {
        log<level::DEBUG>("HostNotifier command response success",
                          entry("PEL_ID=0x%X", id));
        _retryCount = 0;

        _sentPELs.push_back(id);

        _repo.setPELHostTransState(id, TransmissionState::sent);

        // If the host is full, don't send off the next PEL
        if (!_hostFull && !_pelQueue.empty())
        {
            doNewLogNotify();
        }
    }
    else
    {
        log<level::ERR>("PLDM command response failure",
                        entry("PEL_ID=0x%X", id));
        // Retry
        _pelQueue.push_front(id);
        _retryTimer.restartOnce(_hostIface->getReceiveRetryDelay());
    }
}

void HostNotifier::retryTimerExpired()
{
    if (_dataIface.isHostUp())
    {
        log<level::INFO>("Attempting command retry",
                         entry("PEL_ID=0x%X", _pelQueue.front()));
        _retryCount++;
        doNewLogNotify();
    }
}

void HostNotifier::hostFullTimerExpired()
{
    log<level::DEBUG>("Host full timer expired, trying send again");
    doNewLogNotify();
}

void HostNotifier::stopCommand()
{
    _retryCount = 0;

    if (_inProgressPEL != 0)
    {
        _pelQueue.push_front(_inProgressPEL);
        _inProgressPEL = 0;
    }

    if (_retryTimer.isEnabled())
    {
        _retryTimer.setEnabled(false);
    }

    // Ensure the PLDM instance ID is released
    _hostIface->cancelCmd();
}

void HostNotifier::ackPEL(uint32_t id)
{
    _repo.setPELHostTransState(id, TransmissionState::acked);

    // No longer just 'sent', so remove it from the sent list.
    auto sent = std::find(_sentPELs.begin(), _sentPELs.end(), id);
    if (sent != _sentPELs.end())
    {
        _sentPELs.erase(sent);
    }

    // An ack means the host is no longer full
    if (_hostFullTimer.isEnabled())
    {
        _hostFullTimer.setEnabled(false);
    }

    if (_hostFull)
    {
        _hostFull = false;

        log<level::DEBUG>("Host previously full, not anymore after this ack");

        // Start sending PELs again, from the event loop
        if (!_pelQueue.empty())
        {
            scheduleDispatch();
        }
    }
}

void HostNotifier::setHostFull(uint32_t id)
{
    log<level::INFO>("Received Host full indication", entry("PEL_ID=0x%X", id));

    _hostFull = true;

    // This PEL needs to get re-sent
    auto sent = std::find(_sentPELs.begin(), _sentPELs.end(), id);
    if (sent != _sentPELs.end())
    {
        _sentPELs.erase(sent);
        _repo.setPELHostTransState(id, TransmissionState::newPEL);

        if (std::find(_pelQueue.begin(), _pelQueue.end(), id) ==
            _pelQueue.end())
        {
            _pelQueue.push_front(id);
        }
    }

    // The only PELs that will be sent when the
    // host is full is from this timer callback.
    if (!_hostFullTimer.isEnabled())
    {
        log<level::DEBUG>("Starting host full timer");
        _hostFullTimer.restartOnce(_hostIface->getHostFullRetryDelay());
    }
}

void HostNotifier::setBadPEL(uint32_t id)
{
    log<level::ERR>("PEL rejected by the host", entry("PEL_ID=0x%X", id));

    auto sent = std::find(_sentPELs.begin(), _sentPELs.end(), id);
    if (sent != _sentPELs.end())
    {
        _sentPELs.erase(sent);
    }

    _repo.setPELHostTransState(id, TransmissionState::badPEL);
}

} // namespace openpower::pels
