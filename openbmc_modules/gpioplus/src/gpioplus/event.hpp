#pragma once
#include <chrono>
#include <cstdint>
#include <gpioplus/chip.hpp>
#include <gpioplus/handle.hpp>
#include <gpioplus/internal/fd.hpp>
#include <optional>
#include <ratio>
#include <string_view>

namespace gpioplus
{

/** @brief The flags used for registering an event */
struct EventFlags
{
    /** @brief Are rising edge events reported */
    bool rising_edge;
    /** @brief Are falling edge events reported */
    bool falling_edge;

    /** @brief Converts this struct to an int bitfield
     *
     *  @return The int bitfield usable by the syscall interface
     */
    uint32_t toInt() const;
};

/** @class EventInterface
 *  @brief Interface used for providing gpio events
 */
class EventInterface
{
  public:
    virtual ~EventInterface() = default;

    /** @brief Event data read from the gpio line */
    struct Data
    {
        /** @brief The estimate of the time the event occurred */
        std::chrono::duration<uint64_t, std::nano> timestamp;
        /** @brief The identifier of the event */
        uint32_t id;
    };

    virtual std::optional<Data> read() const = 0;
    virtual uint8_t getValue() const = 0;
};

/** @class Event
 *  @brief Handle to a gpio line event
 *  @details Provides a c++ interface for gpio event operations
 */
class Event : public EventInterface
{
  public:
    /** @brief Creates a new gpio line event handler
     *         The underlying implementation of the event is independent of
     *         the provided chip object. It is safe to destroy any of the
     *         provided inputs while this event is alive.
     *
     *  @param[in] chip           - The gpio chip which provides the events
     *  @param[in] line_offset    - The offset of the line generating events
     *  @param[in] handle_flags   - The handle flags applied
     *  @param[in] event_flags    - The event flags applied
     *  @param[in] consumer_label - The functional name of this consumer
     *  @throws std::system_error for underlying syscall failures
     */
    Event(const Chip& chip, uint32_t line_offset, HandleFlags handle_flags,
          EventFlags event_flags, std::string_view consumer_label);

    /** @brief Get the file descriptor used for the handle
     *
     *  @return The gpio handle file descriptor
     */
    const internal::Fd& getFd() const;

    /** @brief Reads an event from the event file descriptor
     *         Follows the read(2) semantics of the underyling file descriptor
     *
     *  @throws std::system_error for underlying syscall failures
     *  @return The value of the event or std::nullopt if the file descriptor
     *          is non-blocking and no event has occurred
     */
    std::optional<Data> read() const override;

    /** @brief Get the current value of the associated line
     *
     *  @throws std::system_error for underlying syscall failures
     *  @return The value of the gpio line
     */
    uint8_t getValue() const override;

  private:
    internal::Fd fd;
};

} // namespace gpioplus
