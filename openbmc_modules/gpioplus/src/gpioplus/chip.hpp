#pragma once
#include <cstdint>
#include <gpioplus/internal/fd.hpp>
#include <gpioplus/internal/sys.hpp>
#include <string>

namespace gpioplus
{

/** @brief Information about the queried gpio chip */
struct ChipInfo
{
    /** @brief Kernel name of the chip */
    std::string name;
    /** @brief Functional name of the chip */
    std::string label;
    /** @brief Number of lines on the chip */
    uint32_t lines;
};

/** @brief Flags pertaining to the gpio line */
struct LineFlags
{
    /** @brief Is the kernel currently using the line */
    bool kernel;
    /** @brief Is the line used for output (otherwise input) */
    bool output;
    /** @brief Is the line value active at low voltage */
    bool active_low;
    /** @brief Is the line an open drain */
    bool open_drain;
    /** @brief Is the line an open source */
    bool open_source;

    /** @brief Converts the syscall flags to this struct
     *
     *  @param[in] flags - The int bitfield of flags
     */
    LineFlags(uint32_t flags);
};

/** @brief Information about the queried gpio line */
struct LineInfo
{
    /** @brief Flags that apply to the line */
    LineFlags flags;
    /** @brief name of the line as specified by the gpio chip */
    std::string name;
    /** @brief the name of the consumer of the line */
    std::string consumer;
};

/** @class Chip
 *  @brief Handle to a gpio chip
 *  @details Provides a c++ interface to gpio chip operations
 */
class Chip
{
  public:
    /** @brief Creates a new chip from chip id
     *         Ids come from the /sys/bus/gpio/devices/gpiochip{id}
     *
     *  @param[in] id  - Id of the chip to open
     *  @param[in] sys - Optional underlying syscall implementation
     */
    Chip(unsigned id, const internal::Sys* sys = &internal::sys_impl);

    /** @brief Gets the information about this chip
     *
     *  @throws std::system_error for underlying syscall failures
     *  @return The chip information
     */
    ChipInfo getChipInfo() const;

    /** @brief Gets the information about a line on the chip
     *
     *  @throws std::system_error for underlying syscall failures
     *  @return The line information
     */
    LineInfo getLineInfo(uint32_t offset) const;

    /** @brief Get the file descriptor associated with the chip
     *
     *  @return The file descriptor
     */
    const internal::Fd& getFd() const;

  private:
    internal::Fd fd;
};

} // namespace gpioplus
