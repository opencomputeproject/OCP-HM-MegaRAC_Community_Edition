#pragma once

#include "additional_data.hpp"
#include "elog_entry.hpp"

#include <phosphor-logging/log.hpp>
#include <queue>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <tuple>

namespace openpower::pels
{

/**
 * @class EventLogger
 *
 * This class handles creating OpenBMC event logs (and thus PELs) from
 * within the PEL extension code.
 *
 * The function to actually create the event log is passed in via the
 * constructor so that different functions can be used when testing.
 *
 * To create the event log, call log() with the appropriate arguments
 * and the log will be created as soon as the flow gets back to the event
 * loop.  If the queue isn't empty after a log is created, the next
 * one will be scheduled to be created from the event loop again.
 *
 * This class does not allow new events to be added while inside the
 * creation function, because if the code added an event log every time
 * it tried to create one, it would do so infinitely.
 */
class EventLogger
{
  public:
    using ADMap = std::map<std::string, std::string>;
    using LogFunction = std::function<void(
        const std::string&, phosphor::logging::Entry::Level, const ADMap&)>;

    static constexpr size_t msgPos = 0;
    static constexpr size_t levelPos = 1;
    static constexpr size_t adPos = 2;
    using EventEntry = std::tuple<std::string, phosphor::logging::Entry::Level,
                                  AdditionalData>;

    EventLogger() = delete;
    ~EventLogger() = default;
    EventLogger(const EventLogger&) = delete;
    EventLogger& operator=(const EventLogger&) = delete;
    EventLogger(EventLogger&&) = delete;
    EventLogger& operator=(EventLogger&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] creator - The function to use to create the event log
     */
    EventLogger(LogFunction creator) :
        _event(sdeventplus::Event::get_default()), _creator(creator)
    {
    }

    /**
     * @brief Adds an event to the queue so that it will be created
     *        as soon as the code makes it back to the event loop.
     *
     * Won't add it to the queue if already inside the create()
     * callback.
     *
     * @param[in] message - The message property of the event log
     * @param[in] severity - The severity level of the event log
     * @param[in] ad - The additional data property of the event log
     */
    void log(const std::string& message,
             phosphor::logging::Entry::Level severity, const AdditionalData& ad)
    {
        if (!_inEventCreation)
        {
            _eventsToCreate.emplace(message, severity, ad);

            if (!_eventSource)
            {
                scheduleCreate();
            }
        }
        else
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "Already in event create callback, skipping new create",
                phosphor::logging::entry("ERROR_NAME=%s", message.c_str()));
        }
    }

    /**
     * @brief Returns the event log queue size.
     *
     * @return size_t - The queue size
     */
    size_t queueSize() const
    {
        return _eventsToCreate.size();
    }

    /**
     * @brief Schedules the create() function to run using the
     *        'defer' sd_event source.
     */
    void scheduleCreate()
    {
        _eventSource = std::make_unique<sdeventplus::source::Defer>(
            _event, std::bind(std::mem_fn(&EventLogger::create), this,
                              std::placeholders::_1));
    }

  private:
    /**
     * @brief Creates an event log and schedules the next one if
     *        there is one.
     *
     * This gets called from the event loop by the sd_event code.
     *
     * @param[in] source - The event source object used
     */
    void create(sdeventplus::source::EventBase& source)
    {
        _eventSource.reset();

        if (_eventsToCreate.empty())
        {
            return;
        }

        auto event = _eventsToCreate.front();
        _eventsToCreate.pop();

        _inEventCreation = true;

        try
        {
            _creator(std::get<msgPos>(event), std::get<levelPos>(event),
                     std::get<adPos>(event).getData());
        }
        catch (std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "EventLogger's create function threw an exception",
                phosphor::logging::entry("ERROR=%s", e.what()));
        }

        _inEventCreation = false;

        if (!_eventsToCreate.empty())
        {
            scheduleCreate();
        }
    }

    /**
     * @brief The sd_event object.
     */
    sdeventplus::Event _event;

    /**
     * @brief The user supplied function to create the event log.
     */
    LogFunction _creator;

    /**
     * @brief Keeps track of if an event is currently being created.
     *
     * Guards against creating new events while creating events.
     */
    bool _inEventCreation = false;

    /**
     * @brief The event source object used for scheduling.
     */
    std::unique_ptr<sdeventplus::source::Defer> _eventSource;

    /**
     * @brief The queue of event logs to create.
     */
    std::queue<EventEntry> _eventsToCreate;
};

} // namespace openpower::pels
