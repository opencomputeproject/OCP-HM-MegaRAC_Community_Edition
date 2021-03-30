#pragma once
#include <gpioplus/internal/sys.hpp>
#include <type_traits>

namespace gpioplus
{
namespace internal
{

/** @class Fd
 *  @brief Holds references to file descriptors
 *  @details Provides RAII semantics for file descriptors
 */
class Fd
{
  public:
    /** @brief Opens a file and holds the file descriptor
     *
     *  @param[in] pathname - The path to the file being opened
     *  @param[in] flags    - Flags passed to open(2)
     *  @param[in] sys      - Optional underlying syscall implementation
     *  @throws std::system_error for underlying syscall failures
     */
    Fd(const char* pathname, int flags, const Sys* sys);

    /** @brief Duplicates and holds a file descriptor
     *        Does not automatically close the input descriptor
     *
     *  @param[in] fd  - File descriptor being duplicated
     *  @param[in] sys - Optional underlying syscall implementation
     *  @throws std::system_error for underlying syscall failures
     */
    Fd(int fd, const Sys* sys);

    /** @brief Holds the input file descriptor
     *        Becomes the sole owner of the file descriptor
     *
     *  @param[in] fd  - File descriptor being duplicated
     *  @param[in]     - Denotes that the fd is directly used
     *  @param[in] sys - Optional underlying syscall implementation
     */
    Fd(int fd, std::false_type, const Sys* sys);

    virtual ~Fd();
    Fd(const Fd& other);
    Fd& operator=(const Fd& other);
    Fd(Fd&& other);
    Fd& operator=(Fd&& other);

    /** @brief Gets the managed file descriptor
     *
     *  @return The file descriptor
     */
    int operator*() const;

    /** @brief Gets the syscall interface implementation
     *
     *  @return The syscall implementation
     */
    const Sys* getSys() const;

    void setBlocking(bool enabled) const;

  private:
    const Sys* sys;
    int fd;

    /** @brief Sets flags on the file descriptor
     *
     *  @param[in] flags - The flags to set
     *  @throws std::system_error for underlying syscall failures
     */
    void setFlags(int flags) const;

    /** @brief Gets the flags from the file descriptor
     *
     *  @throws std::system_error for underlying syscall failures
     *  @return The file descriptor flags
     */
    int getFlags() const;

    /** @brief Cleans up the held file descriptor
     *
     *  @throws std::system_error for underlying syscall failures
     */
    void reset();
};

} // namespace internal
} // namespace gpioplus
