#pragma once

#include <function2/function2.hpp>
#include <sdeventplus/source/base.hpp>
#include <sdeventplus/types.hpp>
#include <signal.h>

namespace sdeventplus
{
namespace source
{

namespace detail
{
class ChildData;
} // namespace detail

/** @class Child
 *  @brief A wrapper around the sd_event_source child type
 *         See sd_event_add_child(3) for more information
 */
class Child : public Base
{
  public:
    using Callback =
        fu2::unique_function<void(Child& source, const siginfo_t* si)>;

    /** @brief Adds a new child source handler to the Event
     *         This type of source defaults to Enabled::Oneshot, and needs to be
     *         reconfigured upon each callback.
     *
     *  @param[in] event    - The event to attach the handler
     *  @param[in] pid      - The pid of the child to monitor
     *  @param[in] options  - An OR-ed mask that determines triggers
     *                        See waitid(2) for further information
     *  @param[in] callback - The function executed on event dispatch
     */
    Child(const Event& event, pid_t pid, int options, Callback&& callback);

    /** @brief Constructs a non-owning child source handler
     *         Does not own the passed reference to the source because
     *         this is meant to be used only as a reference inside an event
     *         source.
     *  @internal
     *
     *  @param[in] other - The source wrapper to copy
     *  @param[in]       - Signifies that this new copy is non-owning
     */
    Child(const Child& other, sdeventplus::internal::NoOwn);

    /** @brief Sets the callback
     *
     *  @param[in] callback - The function executed on event dispatch
     */
    void set_callback(Callback&& callback);

    /** @brief Gets the pid of the child process being watched
     *
     *  @return The child pid
     *  @throws SdEventError for underlying sd_event errors
     */
    pid_t get_pid() const;

  private:
    /** @brief Returns a reference to the source owned child
     *
     *  @return A reference to the child
     */
    detail::ChildData& get_userdata() const;

    /** @brief Returns a reference to the callback executed for this source
     *
     *  @return A reference to the callback
     */
    Callback& get_callback();

    /** @brief Creates a new child source attached to the Event
     *
     *  @param[in] event   - The event to attach the handler
     *  @param[in] pid     - The pid of the child to monitor
     *  @param[in] options - An OR-ed mask that determines triggers
     *  @throws SdEventError for underlying sd_event errors
     *  @return A new sd_event_source
     */
    static sd_event_source* create_source(const Event& event, pid_t pid,
                                          int options);

    /** @brief A wrapper around the callback that can be called from sd-event
     *
     *  @param[in] source   - The sd_event_source associated with the call
     *  @param[in] userdata - The provided userdata for the source
     *  @return 0 on success or a negative errno otherwise
     */
    static int childCallback(sd_event_source* source, const siginfo_t* si,
                             void* userdata);
};

namespace detail
{

class ChildData : public Child, public BaseData
{
  private:
    Child::Callback callback;

  public:
    ChildData(const Child& base, Child::Callback&& callback);

    friend Child;
};

} // namespace detail

} // namespace source
} // namespace sdeventplus
