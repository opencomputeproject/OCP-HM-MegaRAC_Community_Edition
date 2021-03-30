#pragma once

#include <unistd.h>
namespace witherspoon
{
namespace power
{
namespace util
{

/**
 * @class FileDescriptor
 *
 * Closes the file descriptor on destruction
 */
class FileDescriptor
{
  public:
    FileDescriptor() = default;
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    FileDescriptor(FileDescriptor&&) = delete;
    FileDescriptor& operator=(FileDescriptor&&) = delete;

    /**
     * Constructor
     *
     * @param[in] fd - File descriptor
     */
    FileDescriptor(int fd) : fd(fd)
    {}

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
        if (fd >= 0)
        {
            close(fd);
        }

        fd = descriptor;
    }

  private:
    /**
     * File descriptor
     */
    int fd = -1;
};

} // namespace util
} // namespace power
} // namespace witherspoon
