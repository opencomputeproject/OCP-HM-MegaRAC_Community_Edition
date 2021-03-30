#pragma once

#include <unistd.h>
namespace phosphor
{
namespace gpio
{
/** @class FileDescriptor
 *  @brief Responsible for handling file descriptor
 */
class FileDescriptor
{
  private:
    /** @brief File descriptor for the gpio input device */
    int fd = -1;

  public:
    FileDescriptor() = default;
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    FileDescriptor(FileDescriptor&&) = delete;
    FileDescriptor& operator=(FileDescriptor&&) = delete;

    /** @brief Saves File descriptor and uses it to do file operation
     *
     *  @param[in] fd - File descriptor
     */
    FileDescriptor(int fd) : fd(fd)
    {
        // Nothing
    }

    ~FileDescriptor()
    {
        if (fd >= 0)
        {
            close(fd);
        }
    }

    int operator()()
    {
        return fd;
    }

    operator bool() const
    {
        return fd != -1;
    }

    void set(int descriptor)
    {
        fd = descriptor;
    }
};

} // namespace gpio
} // namespace phosphor
