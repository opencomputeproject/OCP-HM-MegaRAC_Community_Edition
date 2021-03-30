#pragma once

#include <cstdint>
#include <function2/function2.hpp>
#include <sdeventplus/source/base.hpp>
#include <systemd/sd-event.h>
#include <type_traits>

namespace sdeventplus
{
namespace source
{

namespace detail
{
class IOData;
} // namespace detail

/** @class IO
 *  @brief A wrapper around the sd_event_source IO type
 *         See sd_event_add_io(3) for more information
 */
class IO : public Base
{
  public:
    using Callback = fu2::unique_function<void(IO&, int fd, uint32_t revents)>;

    /** @brief Adds a new IO source handler to the Event
     *         This type of source defaults to Enabled::On, executing the
     *         callback for each IO epoll event observed.
     *
     *  @param[in] event    - The event to attach the handler
     *  @param[in] fd       - The file descriptor producing the events
     *  @param[in] events   - The event mask passed which determines triggers
     *                        See epoll_ctl(2) for more info on the mask
     *  @param[in] callback - The function executed on event dispatch
     */
    IO(const Event& event, int fd, uint32_t events, Callback&& callback);

    /** @brief Constructs a non-owning io source handler
     *         Does not own the passed reference to the source because
     *         this is meant to be used only as a reference inside an event
     *         source.
     *  @internal
     *
     *  @param[in] other - The source wrapper to copy
     *  @param[in]       - Signifies that this new copy is non-owning
     */
    IO(const IO& event, sdeventplus::internal::NoOwn);

    /** @brief Sets the callback
     *
     *  @param[in] callback - The function executed on event dispatch
     */
    void set_callback(Callback&& callback);

    /** @brief Gets the file descriptor tied to the source
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return The watched file descriptor
     */
    int get_fd() const;

    /** @brief Sets the file descriptor the source watches
     *
     *  @param[in] fd - The file descriptor to watch
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_fd(int fd) const;

    /** @brief Gets the events mask used to determine what
     *         events trigger the callback action
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return The events mask
     */
    uint32_t get_events() const;

    /** @brief Sets the events mask used to determine what events
     *         trigger the callback handler
     *
     *  @param[in] events - The events mask
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_events(uint32_t events) const;

    /** @brief Gets the events mask describing the events
     *         seen in the most recent callback
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return The events mask
     */
    uint32_t get_revents() const;

  private:
    /** @brief Returns a reference to the source owned io
     *
     *  @return A reference to the io
     */
    detail::IOData& get_userdata() const;

    /** @brief Returns a reference to the callback executed for this source
     *
     *  @return A reference to the callback
     */
    Callback& get_callback();

    /** @brief Creates a new IO source attached to the Event
     *
     *  @param[in] event  - The event to attach the handler
     *  @param[in] fd     - The file descriptor producing the events
     *  @param[in] events - The event mask passed which determines triggers
     *                      See epoll_ctl(2) for more info on the mask
     *  @throws SdEventError for underlying sd_event errors
     *  @return A new sd_event_source
     */
    static sd_event_source* create_source(const Event& event, int fd,
                                          uint32_t events);

    /** @brief A wrapper around the callback that can be called from sd-event
     *
     *  @param[in] source   - The sd_event_source associated with the call
     *  @param[in] userdata - The provided userdata for the source
     *  @return 0 on success or a negative errno otherwise
     */
    static int ioCallback(sd_event_source* source, int fd, uint32_t revents,
                          void* userdata);
};

namespace detail
{

class IOData : public IO, public BaseData
{
  private:
    IO::Callback callback;

  public:
    IOData(const IO& base, IO::Callback&& callback);

    friend IO;
};

} // namespace detail

} // namespace source
} // namespace sdeventplus
