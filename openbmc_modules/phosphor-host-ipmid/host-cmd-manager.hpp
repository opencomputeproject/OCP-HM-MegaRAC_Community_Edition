#pragma once

#include <ipmid-host/cmd-utils.hpp>
#include <queue>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/timer.hpp>
#include <tuple>

namespace phosphor
{
namespace host
{
namespace command
{

/** @class
 *  @brief Manages commands that are to be sent to Host
 */
class Manager
{
  public:
    Manager() = delete;
    ~Manager() = default;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    /** @brief Constructs Manager object
     *
     *  @param[in] bus   - dbus handler
     *  @param[in] event - pointer to sd_event
     */
    explicit Manager(sdbusplus::bus::bus& bus);

    /** @brief  Extracts the next entry in the queue and returns
     *          Command and data part of it.
     *
     *  @detail Also calls into the registered handlers so that they can now
     *          send the CommandComplete signal since the interface contract
     *          is that we emit this signal once the message has been
     *          passed to the host (which is required when calling this)
     *
     *          Also, if the queue has more commands, then it will alert the
     *          host
     */
    IpmiCmdData getNextCommand();

    /** @brief  Pushes the command onto the queue.
     *
     *  @detail If the queue is empty, then it alerts the Host. If not,
     *          then it returns and the API documented above will handle
     *          the commands in Queue.
     *
     *  @param[in] command - tuple of <IPMI command, data, callback>
     */
    void execute(CommandHandler command);

  private:
    /** @brief Check if anything in queue and alert host if so */
    void checkQueueAndAlertHost();

    /** @brief  Call back interface on message timeouts to host.
     *
     *  @detail When this happens, a failure message would be sent
     *          to all the commands that are in the Queue and queue
     *          will be purged
     */
    void hostTimeout();

    /** @brief Clears the command queue
     *
     *  @detail Clears the command queue and calls all callbacks
     *          specifying the command wasn't successful.
     */
    void clearQueue();

    /** @brief Clears the command queue on a power on
     *
     *  @detail The properties changed handler for the
     *          RequestedHostTransition property.  When this property
     *          changes to 'On', this function will purge the command
     *          queue.
     *
     *          This is done to avoid having commands that were issued
     *          before the host powers on from getting sent to the host,
     *          either due to race conditions around state transitions
     *          or from a user doing something like requesting an already
     *          powered off system to power off again and then immediately
     *          requesting a power on.
     *
     *  @param[in] msg - the sdbusplus message containing the property
     */
    void clearQueueOnPowerOn(sdbusplus::message::message& msg);

    /** @brief Reference to the dbus handler */
    sdbusplus::bus::bus& bus;

    /** @brief Queue to store the requested commands */
    std::queue<CommandHandler> workQueue{};

    /** @brief Timer for commands to host */
    phosphor::Timer timer;

    /** @brief Match handler for the requested host state */
    sdbusplus::bus::match_t hostTransitionMatch;
};

} // namespace command
} // namespace host
} // namespace phosphor
