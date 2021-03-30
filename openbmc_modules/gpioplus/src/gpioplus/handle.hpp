#pragma once
#include <cstdint>
#include <gpioplus/chip.hpp>
#include <gpioplus/internal/fd.hpp>
#include <string_view>
#include <vector>

namespace gpioplus
{

/** @brief Flags to set when taking a handle */
struct HandleFlags
{
    /** @brief Is the line used for output (otherwise input) */
    bool output;
    /** @brief Is the line value active at low voltage */
    bool active_low;
    /** @brief Is the line an open drain */
    bool open_drain;
    /** @brief Is the line an open source */
    bool open_source;

    HandleFlags() = default;

    /** @brief Creates handle flags from the gpio line flags
     *
     *  @param[in] line_flags - Line flags used for population
     */
    explicit HandleFlags(LineFlags line_flags);

    /** @brief Converts this struct to an int bitfield
     *
     *  @return The int bitfield usable by the syscall interface
     */
    uint32_t toInt() const;
};

/** @class HandleInterface
 *  @brief Handle interface to provide a set of methods required to exist in
 * derived objects.
 */
class HandleInterface
{
  public:
    virtual ~HandleInterface() = default;

    virtual std::vector<uint8_t> getValues() const = 0;
    virtual void getValues(std::vector<uint8_t>& values) const = 0;
    virtual void setValues(const std::vector<uint8_t>& values) const = 0;
};

/** @class Handle
 *  @brief Handle to a gpio line handle
 *  @details Provides a c++ interface for gpio handle operations
 */
class Handle : public HandleInterface
{
  public:
    /** @brief Per line information used to construct a handle */
    struct Line
    {
        /** @brief Offset of the line on the gpio chip */
        uint32_t offset;
        /** @brief Default output value of the line */
        uint8_t default_value;
    };

    /** @brief Creates a new gpio handle
     *         The underlying implementation of the handle is independent of
     *         the provided chip object. It is safe to destroy any of the
     *         provided inputs while this handle is alive.
     *
     *  @param[in] chip           - The gpio chip which provides the handle
     *  @param[in] lines          - A collection of lines the handle provides
     *  @param[in] flags          - The flags applied to all lines
     *  @param[in] consumer_label - The functional name of this consumer
     *  @throws std::system_error for underlying syscall failures
     */
    Handle(const Chip& chip, const std::vector<Line>& lines, HandleFlags flags,
           std::string_view consumer_label);

    /** @brief Get the file descriptor used for the handle
     *
     *  @return The gpio handle file descriptor
     */
    const internal::Fd& getFd() const;

    /** @brief Get the current values of all associated lines
     *
     *  @throws std::system_error for underlying syscall failures
     *  @return The values of the gpio lines
     */
    std::vector<uint8_t> getValues() const override;

    /** @brief Gets the current values of all associated lines
     *
     *  @param[out] values - The values of the gpio lines
     *  @throws std::system_error for underlying syscall failures
     */
    void getValues(std::vector<uint8_t>& values) const override;

    /** @brief Sets the current values of all associated lines
     *
     *  @param[in] values - The new values of the gpio lines
     *  @throws std::system_error for underlying syscall failures
     */
    void setValues(const std::vector<uint8_t>& values) const override;

  private:
    internal::Fd fd;
    uint32_t nlines;
};

} // namespace gpioplus
