#pragma once

#include "data_interface.hpp"

#include <stdint.h>

#include <chrono>
#include <functional>
#include <phosphor-logging/log.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/io.hpp>

namespace openpower
{
namespace pels
{

/**
 * @brief Return codes from sending a command
 */
enum class CmdStatus
{
    success,
    failure
};

/**
 * @brief Return codes from the command response
 */
enum class ResponseStatus
{
    success,
    failure
};

/**
 * @class HostInterface
 *
 * An abstract base class for sending the 'New PEL available' command
 * to the host.  Used so that the PLDM interfaces can be mocked for
 * testing the HostNotifier code.  The response to this command is
 * asynchronous, with the intent that other code registers a callback
 * function to run when the response is received.
 */
class HostInterface
{
  public:
    HostInterface() = delete;
    virtual ~HostInterface() = default;
    HostInterface(const HostInterface&) = default;
    HostInterface& operator=(const HostInterface&) = default;
    HostInterface(HostInterface&&) = default;
    HostInterface& operator=(HostInterface&&) = default;

    /**
     * @brief Constructor
     *
     * @param[in] event - The sd_event object pointer
     * @param[in] dataIface - The DataInterface object
     */
    HostInterface(sd_event* event, DataInterfaceBase& dataIface) :
        _event(event), _dataIface(dataIface)
    {
    }

    /**
     * @brief Pure virtual function for sending the 'new PEL available'
     *        asynchronous command to the host.
     *
     * @param[in] id - The ID of the new PEL
     * @param[in] size - The size of the new PEL
     *
     * @return CmdStatus - If the send was successful or not
     */
    virtual CmdStatus sendNewLogCmd(uint32_t id, uint32_t size) = 0;

    /**
     * @brief Returns the amount of time to wait before retrying after
     *        a failed send command.
     *
     * @return milliseconds - The amount of time to wait
     */
    virtual std::chrono::milliseconds getSendRetryDelay() const
    {
        return _defaultSendRetryDelay;
    }

    /**
     * @brief Returns the amount of time to wait before retrying after
     *        a command receive.
     *
     * @return milliseconds - The amount of time to wait
     */
    virtual std::chrono::milliseconds getReceiveRetryDelay() const
    {
        return _defaultReceiveRetryDelay;
    }

    /**
     * @brief Returns the amount of time to wait before retrying if the
     *        host firmware's PEL storage was full and it can't store
     *        any more logs until it is freed up somehow.
     *
     * In this class to help with mocking.
     *
     * @return milliseconds - The amount of time to wait
     */
    virtual std::chrono::milliseconds getHostFullRetryDelay() const
    {
        return _defaultHostFullRetryDelay;
    }

    using ResponseFunction = std::function<void(ResponseStatus)>;

    /**
     * @brief Sets the function to call on the command receive.
     *
     * The success/failure status is passed to the function.
     *
     * @param[in] func - The callback function
     */
    void setResponseFunction(ResponseFunction func)
    {
        _responseFunc = std::move(func);
    }

    /**
     * @brief Call the response function
     *
     * @param[in] status - The status given to the function
     */
    void callResponseFunc(ResponseStatus status)
    {
        if (_responseFunc)
        {
            try
            {
                (*_responseFunc)(status);
            }
            catch (const std::exception& e)
            {
                using namespace phosphor::logging;
                log<level::ERR>(
                    "Host iface response callback threw an exception",
                    entry("ERROR=%s", e.what()));
            }
        }
    }

    /**
     * @brief Returns the event object in use
     *
     * @return sdeventplus::Event& - The event object
     */
    sdeventplus::Event& getEvent()
    {
        return _event;
    }

    /**
     * @brief Pure virtual function to cancel an in-progress command
     *
     * 'In progress' means after the send but before the receive
     */
    virtual void cancelCmd() = 0;

    /**
     * @brief Says if the command is in progress (after send/before receive)
     *
     * @return bool - If command is in progress
     */
    bool cmdInProgress() const
    {
        return _inProgress;
    }

  protected:
    /**
     * @brief Pure virtual function for implementing the asynchronous
     *        command response callback.
     *
     * @param[in] io - The sdeventplus IO object that the callback is
     *                 invoked from.
     * @param[in] fd - The file descriptor being used
     * @param[in] revents - The event status bits
     */
    virtual void receive(sdeventplus::source::IO& io, int fd,
                         uint32_t revents) = 0;

    /**
     * @brief An optional function to call on a successful command response.
     */
    std::optional<ResponseFunction> _responseFunc;

    /**
     * @brief The sd_event wrapper object needed for response callbacks
     */
    sdeventplus::Event _event;

    /**
     * @brief The DataInterface object
     */
    DataInterfaceBase& _dataIface;

    /**
     * @brief Tracks status of after a command is sent and before the
     *        response is received.
     */
    bool _inProgress = false;

  private:
    /**
     * @brief The default amount of time to wait before retrying
     *        a failed send.
     */
    const std::chrono::milliseconds _defaultSendRetryDelay{1000};

    /**
     * @brief The default amount of time to wait
     *        before retrying after a failed receive.
     */
    const std::chrono::milliseconds _defaultReceiveRetryDelay{1000};

    /**
     * @brief The default amount of time to wait when the host said it
     *        was full before sending the PEL again.
     */
    const std::chrono::milliseconds _defaultHostFullRetryDelay{60000};
};

} // namespace pels
} // namespace openpower
