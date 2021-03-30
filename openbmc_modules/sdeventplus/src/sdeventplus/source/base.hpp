#pragma once

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <function2/function2.hpp>
#include <functional>
#include <memory>
#include <sdeventplus/event.hpp>
#include <sdeventplus/internal/utils.hpp>
#include <sdeventplus/types.hpp>
#include <stdplus/handle/copyable.hpp>
#include <systemd/sd-bus.h>
#include <type_traits>
#include <utility>

namespace sdeventplus
{
namespace source
{

/** @class Enabled
 *  @brief Mapping of sdeventplus source enable values to the sd-event
 *         equivalent
 */
enum class Enabled
{
    Off = SD_EVENT_OFF,
    On = SD_EVENT_ON,
    OneShot = SD_EVENT_ONESHOT,
};

namespace detail
{
class BaseData;
} // namespace detail

/** @class Base
 *  @brief The base class for all sources implementing common source methods
 *         Not instantiated directly by end users
 */
class Base
{
  public:
    using Callback = fu2::unique_function<void(Base& source)>;

    Base(Base&& other) = default;
    Base& operator=(Base&& other) = default;
    Base(const Base& other) = default;
    Base& operator=(const Base& other) = default;
    virtual ~Base() = default;

    /** @brief Gets the underlying sd_event_source
     *
     *  @return The sd_event_source
     */
    sd_event_source* get() const;

    /** @brief Gets the associated Event object
     *
     *  @return The Event
     */
    const Event& get_event() const;

    /** @brief Gets the description of the source
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return The c-string description or a nullptr if none exists
     */
    const char* get_description() const;

    /** @brief Sets the description of the source
     *
     *  @param[in] description - The c-string description
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_description(const char* description) const;

    /** @brief Sets the callback associated with the source to be performed
     *         before the event loop goes to sleep, waiting for new events
     *
     *  @param[in] callback - Function run for preparation of the source
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_prepare(Callback&& callback);

    /** @brief Whether or not the source has any pending events that have
     *         not been dispatched yet.
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return 'true' if the source has pending events
     *          'false' otherwise
     */
    bool get_pending() const;

    /** @brief Gets the priority of the source relative to other sources
     *         The lower the priority the more important the source
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return A 64 bit integer representing the dispatch priority
     */
    int64_t get_priority() const;

    /** @brief Sets the priority of the source relative to other sources
     *         The lower the priority the more important the source
     *
     *  @param[in] priority - A 64 bit integer representing the priority
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_priority(int64_t priority) const;

    /** @brief Determines the enablement value of the source
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return The enabled status of the source
     */
    Enabled get_enabled() const;

    /** @brief Sets the enablement value of the source
     *
     *  @param[in] enabled - The new state of the source
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_enabled(Enabled enabled) const;

    /** @brief Determines the floating nature of the source
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return The enabled status of the source
     */
    bool get_floating() const;

    /** @brief Sets the floating nature of the source
     *         If set to true, the source will continue to run after the
     *         destruction of this handle.
     *
     *  @param[in] b - Whether or not the source should float
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_floating(bool b) const;

  protected:
    Event event;

    /** @brief Constructs a basic event source wrapper
     *         Owns the passed reference to the source
     *         This ownership is exception safe and will properly free the
     *         source in the case of an exception during construction
     *
     *  @param[in] event  - The event associated with the source
     *  @param[in] source - The underlying sd_event_source wrapped
     *  @param[in]        - Signifies that ownership is being transfered
     */
    Base(const Event& event, sd_event_source* source, std::false_type);

    /** @brief Constructs a basic non-owning event source wrapper
     *         Does not own the passed reference to the source because
     *         this is meant to be used only as a reference inside an event
     *         source.
     *  @internal
     *
     *  @param[in] other - The source wrapper to copy
     *  @param[in]       - Signifies that this new copy is non-owning
     */
    Base(const Base& other, sdeventplus::internal::NoOwn);

    /** @brief Sets the userdata of the source to the passed in source
     *         This needs to be called by all source implementors.
     *
     *  @param[in] data - The data stored in the userdata slot.
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_userdata(std::unique_ptr<detail::BaseData> data) const;

    /** @brief Get the heap allocated version of the Base
     *
     *  @return A reference to the Base
     */
    detail::BaseData& get_userdata() const;

    /** @brief Returns a reference to the prepare callback executed for this
     *         source
     *
     *  @return A reference to the callback, this should be checked to make sure
     *          the callback is valid as there is no guarantee
     */
    Callback& get_prepare();

    /** @brief A helper for subclasses to trivially wrap a c++ style callback
     *         to be called from the sd-event c library
     *
     *  @param[in] name     - The name of the callback for use in error messages
     *  @param[in] source   - The sd_event_source provided by sd-event
     *  @param[in] userdata - The userdata provided by sd-event
     *  @param[in] args...  - Extra arguments to pass to the callaback
     *  @return An negative errno on error, or 0 on success
     */
    template <typename Callback, class Data, auto getter, typename... Args>
    static int sourceCallback(const char* name, sd_event_source*,
                              void* userdata, Args&&... args)
    {
        if (userdata == nullptr)
        {
            fprintf(stderr, "sdeventplus: %s: Missing userdata\n", name);
            return -EINVAL;
        }
        Data& data =
            static_cast<Data&>(*reinterpret_cast<detail::BaseData*>(userdata));
        Callback& callback = std::invoke(getter, data);
        return internal::performCallback(name, callback, std::ref(data),
                                         std::forward<Args>(args)...);
    }

  private:
    static sd_event_source* ref(sd_event_source* const& source,
                                const internal::SdEvent*& sdevent, bool& owned);
    static void drop(sd_event_source*&& source,
                     const internal::SdEvent*& sdevent, bool& owned);

    stdplus::Copyable<sd_event_source*, const internal::SdEvent*,
                      bool>::Handle<drop, ref>
        source;

    /** @brief A wrapper around deleting the heap allocated base class
     *         This is needed for calls from sd_event destroy callbacks.
     *
     * @param[in] userdata - The provided userdata for the source
     */
    static void destroy_userdata(void* userdata);

    /** @brief A wrapper around the callback that can be called from sd-event
     *
     * @param[in] source   - The sd_event_source associated with the call
     * @param[in] userdata - The provided userdata for the source
     * @return 0 on success or a negative errno otherwise
     */
    static int prepareCallback(sd_event_source* source, void* userdata);
};

namespace detail
{

class BaseData : public Base
{
  private:
    Base::Callback prepare;

  public:
    BaseData(const Base& base);

    friend Base;
};

} // namespace detail

} // namespace source
} // namespace sdeventplus
