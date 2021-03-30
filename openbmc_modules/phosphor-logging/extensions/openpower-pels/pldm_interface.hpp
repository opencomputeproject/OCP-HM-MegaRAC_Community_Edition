#pragma once

#include "host_interface.hpp"

#include <libpldm/pldm.h>

#include <chrono>
#include <memory>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/source/io.hpp>
#include <sdeventplus/utility/timer.hpp>

namespace openpower::pels
{

/**
 * @class PLDMInterface
 *
 * This class handles sending the 'new file available' PLDM
 * command to the host to notify it of a new PEL's ID and size.
 *
 * The command response is asynchronous.
 */
class PLDMInterface : public HostInterface
{
  public:
    PLDMInterface() = delete;
    PLDMInterface(const PLDMInterface&) = default;
    PLDMInterface& operator=(const PLDMInterface&) = default;
    PLDMInterface(PLDMInterface&&) = default;
    PLDMInterface& operator=(PLDMInterface&&) = default;

    /**
     * @brief Constructor
     *
     * @param[in] event - The sd_event object pointer
     * @param[in] dataIface - The DataInterface object
     */
    PLDMInterface(sd_event* event, DataInterfaceBase& dataIface) :
        HostInterface(event, dataIface),
        _receiveTimer(
            event,
            std::bind(std::mem_fn(&PLDMInterface::receiveTimerExpired), this))
    {
        sd_bus_default(&_bus);

        readEID();
    }

    /**
     * @brief Destructor
     */
    ~PLDMInterface();

    /**
     * @brief Kicks off the send of the 'new file available' command
     *        to send up the ID and size of the new PEL.
     *
     * It starts by issuing the async D-Bus method call to read the
     * instance ID.
     *
     * @param[in] id - The PEL ID
     * @param[in] size - The PEL size in bytes
     *
     * @return CmdStatus - the success/fail status of the send
     */
    CmdStatus sendNewLogCmd(uint32_t id, uint32_t size) override;

    /**
     * @brief Cancels waiting for a command response
     *
     * This will clear the instance ID so the next command
     * will request a new one.
     */
    void cancelCmd() override;

    /**
     * @brief Cleans up so that a new command is ready to be sent.
     *
     * Does not clear the instance ID.
     */
    void cleanupCmd();

    /**
     * @brief Gets called on the async D-Bus method response to
     *        getting the PLDM instance ID.
     *
     * It will read the instance ID out of the message and then
     * continue on with sending the new log command to the host.
     *
     * @param[in] msg - The message containing the instance ID.
     */
    void instanceIDCallback(sd_bus_message* msg);

  private:
    /**
     * @brief The asynchronous callback for getting the response
     *        of the 'new file available' command.
     *
     * Calls the response callback that is registered.
     *
     * @param[in] io - The event source object
     * @param[in] fd - The FD used
     * @param[in] revents - The event bits
     */
    void receive(sdeventplus::source::IO& io, int fd,
                 uint32_t revents) override;

    /**
     * @brief Function called when the receive timer expires.
     *
     * This is considered a failure and so will invoke the
     * registered response callback function with a failure
     * indication.
     */
    void receiveTimerExpired();

    /**
     * @brief Configures the sdeventplus::source::IO object to
     *        call receive() on EPOLLIN activity on the PLDM FD
     *        which is used for command responses.
     */
    void registerReceiveCallback();

    /**
     * @brief Reads the MCTP endpoint ID out of a file
     */
    void readEID();

    /**
     * @brief Opens the PLDM file descriptor
     */
    void open();

    /**
     * @brief Makes the async D-Bus method call to read the PLDM instance
     *        ID needed to send PLDM commands.
     */
    void startReadInstanceID();

    /**
     * @brief Encodes and sends the PLDM 'new file available' cmd
     */
    void doSend();

    /**
     * @brief Closes the PLDM file descriptor
     */
    void closeFD();

    /**
     * @brief Kicks off the send of the 'new file available' command
     *        to send the ID and size of a PEL after the instance ID
     *        has been retrieved.
     */
    void startCommand();

    /**
     * @brief The MCTP endpoint ID
     */
    mctp_eid_t _eid;

    /**
     * @brief The PLDM instance ID of the current command
     *
     * A new ID will be used for every command.
     *
     * If there are command failures, the same instance ID can be
     * used on retries only if the host didn't respond.
     */
    std::optional<uint8_t> _instanceID;

    /**
     * @brief The PLDM command file descriptor for the current command
     */
    int _fd = -1;

    /**
     * @brief The event object for handling callbacks on the PLDM FD
     */
    std::unique_ptr<sdeventplus::source::IO> _source;

    /**
     * @brief A timer to only allow a certain amount of time for the
     *        async PLDM receive before it is considered a failure.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> _receiveTimer;

    /**
     * @brief The command timeout value
     */
    const std::chrono::milliseconds _receiveTimeout{10000};

    /**
     * @brief The D-Bus connection needed for the async method call.
     */
    sd_bus* _bus = nullptr;

    /**
     * @brief The ID of the PEL to notify the host of.
     */
    uint32_t _pelID = 0;

    /**
     * @brief The size of the PEL to notify the host of.
     */
    uint32_t _pelSize = 0;
};

} // namespace openpower::pels
