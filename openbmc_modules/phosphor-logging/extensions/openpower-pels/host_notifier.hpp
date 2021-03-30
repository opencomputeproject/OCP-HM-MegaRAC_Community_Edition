#pragma once

#include "host_interface.hpp"
#include "pel.hpp"
#include "repository.hpp"

#include <deque>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/utility/timer.hpp>

namespace openpower::pels
{

/**
 * @class HostNotifier
 *
 * This class handles notifying the host firmware of new PELs.
 *
 * It uses the Repository class's subscription feature to be
 * notified about new PELs.
 *
 * Some PELs do not need to be sent - see enqueueRequired() and
 * notifyRequired().
 *
 * The high level good path flow for sending a single PEL is:
 *
 * 1) Send the ID and size of the new PEL to the host.
 *   - The command response is asynchronous.
 *
 * 2) The host reads the raw PEL data (outside of this class).
 *
 * 3) The host sends the PEL to the OS, and then sends an AckPEL
 *    PLDM command to the PLDM daemon, who makes a D-Bus method
 *    call to this daemon, which calls HostNotifer::ackPEL().
 *
 *    After this, a PEL never needs to be sent again, but if the
 *    host is rebooted before the ack comes it will.
 *
 * The host firmware has a finite amount of space to store PELs before
 * sending to the OS, and it's possible it will fill up.  In this case,
 * the AckPEL command will have a special response that will tell the
 * PLDM daemon to call  HostReject D-Bus method on this daemon instead
 * which will invoke HostNotifier::setHostFull(). This will stop new
 * PELs from being sent, and the first PEL that hits this will have
 * a timer set to retry again later.
 */
class HostNotifier
{
  public:
    HostNotifier() = delete;
    HostNotifier(const HostNotifier&) = delete;
    HostNotifier& operator=(const HostNotifier&) = delete;
    HostNotifier(HostNotifier&&) = delete;
    HostNotifier& operator=(HostNotifier&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] repo - The PEL repository object
     * @param[in] dataIface - The data interface object
     * @param[in] hostIface - The host interface object
     */
    HostNotifier(Repository& repo, DataInterfaceBase& dataIface,
                 std::unique_ptr<HostInterface> hostIface);

    /**
     * @brief Destructor
     */
    ~HostNotifier();

    /**
     * @brief Returns the PEL queue size.
     *
     * For testing.
     *
     * @return size_t - The queue size
     */
    size_t queueSize() const
    {
        return _pelQueue.size();
    }

    /**
     * @brief Specifies if the PEL needs to go onto the queue to be
     *        set to the host.
     *
     * Only returns false if:
     *  - Already acked by the host (or they didn't like it)
     *  - Hidden and the HMC already got it
     *  - The 'do not report to host' bit is set
     *
     * @param[in] id - The PEL ID
     *
     * @return bool - If enqueue is required
     */
    bool enqueueRequired(uint32_t id) const;

    /**
     * @brief If the host still needs to be notified of the PEL
     *        at the time of the notification.
     *
     * Only returns false if:
     *  - Already acked by the host
     *  - It's hidden, and the HMC already got or will get it.
     *
     * @param[in] id - The PEL ID
     *
     * @return bool - If the notify is required
     */
    bool notifyRequired(uint32_t id) const;

    /**
     * @brief Called when the host sends the 'ack' PLDM command.
     *
     * This means the PEL never needs to be sent up again.
     *
     * If the host was previously full, it is also an indication
     * it no longer is.
     *
     * @param[in] id - The PEL ID
     */
    void ackPEL(uint32_t id);

    /**
     * @brief Called when the host does not have room for more
     *        PELs at this time.
     *
     * This can happen when an OS isn't running yet, and the
     * staging area to hold the PELs before sending them up
     * to the OS is full.  This will stop future PEls from being
     * sent up, as explained below.
     *
     * The PEL with this ID will need to be sent again, so its
     * state is set back to 'new', and it is removed from the list
     * of already sent PELs.
     *
     * A timer will be started, if it isn't already running, to
     * issue another send in the hopes that space has been freed
     * up by then (Receiving an ackPEL response is also an
     * indication of this if there happened to have been other
     * PELs in flight).
     *
     * @param[in] id - The PEL ID
     */
    void setHostFull(uint32_t id);

    /**
     * @brief Called when the host receives a malformed PEL.
     *
     * Ideally this will never happen, as the Repository
     * class already purges malformed PELs.
     *
     * The PEL should never be sent up again.
     *
     * @param[in] id - The PEL ID
     */
    void setBadPEL(uint32_t id);

  private:
    /**
     * @brief This function gets called by the Repository class
     *        when a new PEL is added to it.
     *
     * This function puts the PEL on the queue to be sent up if it
     * needs it, and possibly dispatch the send if the conditions call
     * for it.
     *
     * @param[in] pel - The new PEL
     */
    void newLogCallback(const PEL& pel);

    /**
     * @brief This function gets called by the Repository class
     *        when a PEL is deleted.
     *
     * The deleted ID will be removed from the PEL queue and the
     * sent list.
     *
     * @param[in] id - The deleted PEL ID
     */
    void deleteLogCallback(uint32_t id);

    /**
     * @brief This function runs on every existing PEL at startup
     *        and puts the PEL on the queue to send if necessary.
     *
     * @param[in] pel - The PEL
     *
     * @return bool - This is an indicator to the Repository::for_each
     *                function to traverse every PEL.  Always false.
     */
    bool addPELToQueue(const PEL& pel);

    /**
     * @brief Takes the first PEL from the queue that needs to be
     *        sent, and issues the send if conditions are right.
     */
    void doNewLogNotify();

    /**
     * @brief Creates the event object to handle sending the PLDM
     *        command from the event loop.
     */
    void scheduleDispatch();

    /**
     * @brief Kicks off the PLDM send, but called from the event
     *        loop.
     *
     * @param[in] source - The event source object
     */
    void dispatch(sdeventplus::source::EventBase& source);

    /**
     * @brief Called when the host changes state.
     *
     * If the new state is host up and there are PELs to send, it
     * will trigger the first command.  If the new state is off, then
     * it will transfer any PELs that were sent but not acked yet back
     * to the queue to be sent again.
     *
     * @param[in] hostUp - The new host state
     */
    void hostStateChange(bool hostUp);

    /**
     * @brief The callback function invoked after the asynchronous
     *        PLDM receive function is complete.
     *
     * If the command was successful, the state of that PEL will
     * be set to 'sent', and the next send will be triggered.
     *
     * If the command failed, a retry timer will be started so it
     * can be sent again.
     *
     * @param[in] status - The response status
     */
    void commandResponse(ResponseStatus status);

    /**
     * @brief The function called when the command failure retry
     *        time is up.
     *
     * It will issue a send of the previous PEL and increment the
     * retry count.
     */
    void retryTimerExpired();

    /**
     * @brief The function called when the 'host full' retry timer
     *        expires.
     *
     * This will re-issue a command to try again with the PEL at
     * the front of the queue.
     */
    void hostFullTimerExpired();

    /**
     * @brief Stops an in progress command
     *
     * In progress meaning after the send but before the response.
     */
    void stopCommand();

    /**
     * @brief The PEL repository object
     */
    Repository& _repo;

    /**
     * @brief The data interface object
     */
    DataInterfaceBase& _dataIface;

    /**
     * @brief Base class pointer for the host command interface
     */
    std::unique_ptr<HostInterface> _hostIface;

    /**
     * @brief The list of PEL IDs that need to be sent.
     */
    std::deque<uint32_t> _pelQueue;

    /**
     * @brief The list of IDs that were sent, but not acked yet.
     *
     * These move back to _pelQueue on a power off.
     */
    std::vector<uint32_t> _sentPELs;

    /**
     * @brief The ID the PEL where the notification has
     *        been kicked off but the asynchronous response
     *        hasn't been received yet.
     */
    uint32_t _inProgressPEL = 0;

    /**
     * @brief The command retry count
     */
    size_t _retryCount = 0;

    /**
     * @brief Indicates if the host has said it is full and does not
     *        currently have the space for more PELs.
     */
    bool _hostFull = false;

    /**
     * @brief The command retry timer.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> _retryTimer;

    /**
     * @brief The host full timer, used to retry sending a PEL if the host
     *        said it is full.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> _hostFullTimer;

    /**
     * @brief The object used to dispatch a new PEL send from the
     *        event loop, so the calling function can be returned from
     *        first.
     */
    std::unique_ptr<sdeventplus::source::Defer> _dispatcher;
};

} // namespace openpower::pels
