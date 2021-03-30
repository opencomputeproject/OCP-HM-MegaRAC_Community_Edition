#pragma once

#include <function2/function2.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/base.hpp>
#include <sdeventplus/types.hpp>
#include <sys/signalfd.h>

namespace sdeventplus
{
namespace source
{

namespace detail
{
class SignalData;
} // namespace detail

/** @class Signal
 *  @brief A wrapper around the sd_event_source signal type
 *         See sd_event_add_signal(3) for more information
 */
class Signal : public Base
{
  public:
    /** @brief Type of the user provided callback function */
    using Callback = fu2::unique_function<void(
        Signal& source, const struct signalfd_siginfo* si)>;

    /** @brief Creates a new signal event source on the provided event loop
     *         This type of source defaults to Enabled::On, executing the
     *         callback for each signal observed. You are required to block
     *         the signal in all threads prior to creating this source.
     *
     *  @param[in] event    - The event to attach the handler
     *  @param[in] sig      - Signum to watch, see signal(7)
     *  @param[in] callback - The function executed on event dispatch
     *  @throws SdEventError for underlying sd_event errors
     */
    Signal(const Event& event, int sig, Callback&& callback);

    /** @brief Constructs a non-owning signal source handler
     *         Does not own the passed reference to the source because
     *         this is meant to be used only as a reference inside an event
     *         source.
     *  @internal
     *
     *  @param[in] other - The source wrapper to copy
     *  @param[in]       - Signifies that this new copy is non-owning
     *  @throws SdEventError for underlying sd_event errors
     */
    Signal(const Signal& other, sdeventplus::internal::NoOwn);

    /** @brief Sets the callback
     *
     *  @param[in] callback - The function executed on event dispatch
     */
    void set_callback(Callback&& callback);

    /** @brief Gets the signum watched by the source
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return Integer signum
     */
    int get_signal() const;

  private:
    /** @brief Returns a reference to the source owned signal
     *
     *  @return A reference to the signal
     */
    detail::SignalData& get_userdata() const;

    /** @brief Returns a reference to the callback executed for this source
     *
     *  @return A reference to the callback
     */
    Callback& get_callback();

    /** @brief Creates a new signal source attached to the Event
     *
     *  @param[in] event    - The event to attach the handler
     *  @param[in] sig      - Signum to watch, see signal(7)
     *  @param[in] callback - The function executed on event dispatch
     *  @throws SdEventError for underlying sd_event errors
     *  @return A new sd_event_source
     */
    static sd_event_source* create_source(const Event& event, int sig);

    /** @brief A wrapper around the callback that can be called from sd-event
     *
     *  @param[in] source   - The sd_event_source associated with the call
     *  @param[in] userdata - The provided userdata for the source
     *  @return 0 on success or a negative errno otherwise
     */
    static int signalCallback(sd_event_source* source,
                              const struct signalfd_siginfo* si,
                              void* userdata);
};

namespace detail
{

class SignalData : public Signal, public BaseData
{
  private:
    Signal::Callback callback;

  public:
    SignalData(const Signal& base, Signal::Callback&& callback);

    friend Signal;
};

} // namespace detail

} // namespace source
} // namespace sdeventplus
