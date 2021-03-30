#pragma once

/* NOTE: IIRC, wak@ is working on exposing some of this in stdplus, so we can
 * transition when that's ready.
 *
 * Copied some from gpioplus to enable unit-testing of lpc nuvoton and later
 * other pieces.
 */

#include <netdb.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <system_error>

namespace internal
{

/**
 * @class Sys
 * @brief Overridable direct syscall interface
 */
class Sys
{
  public:
    virtual ~Sys() = default;

    virtual int open(const char* pathname, int flags) const = 0;
    virtual int read(int fd, void* buf, std::size_t count) const = 0;
    virtual int pread(int fd, void* buf, std::size_t count,
                      off_t offset) const = 0;
    virtual int pwrite(int fd, const void* buf, std::size_t count,
                       off_t offset) const = 0;
    virtual int close(int fd) const = 0;
    virtual void* mmap(void* addr, std::size_t length, int prot, int flags,
                       int fd, off_t offset) const = 0;
    virtual int munmap(void* addr, std::size_t length) const = 0;
    virtual int getpagesize() const = 0;
    virtual int ioctl(int fd, unsigned long request, void* param) const = 0;
    virtual int poll(struct pollfd* fds, nfds_t nfds, int timeout) const = 0;
    virtual int socket(int domain, int type, int protocol) const = 0;
    virtual int connect(int sockfd, const struct sockaddr* addr,
                        socklen_t addrlen) const = 0;
    virtual ssize_t sendfile(int out_fd, int in_fd, off_t* offset,
                             size_t count) const = 0;
    virtual int getaddrinfo(const char* node, const char* service,
                            const struct addrinfo* hints,
                            struct addrinfo** res) const = 0;
    virtual void freeaddrinfo(struct addrinfo* res) const = 0;
    virtual std::int64_t getSize(const char* pathname) const = 0;
};

/**
 * @class SysImpl
 * @brief syscall concrete implementation
 * @details Passes through all calls to the normal linux syscalls
 */
class SysImpl : public Sys
{
  public:
    int open(const char* pathname, int flags) const override;
    int read(int fd, void* buf, std::size_t count) const override;
    int pread(int fd, void* buf, std::size_t count,
              off_t offset) const override;
    int pwrite(int fd, const void* buf, std::size_t count,
               off_t offset) const override;
    int close(int fd) const override;
    void* mmap(void* addr, std::size_t length, int prot, int flags, int fd,
               off_t offset) const override;
    int munmap(void* addr, std::size_t length) const override;
    int getpagesize() const override;
    int ioctl(int fd, unsigned long request, void* param) const override;
    int poll(struct pollfd* fds, nfds_t nfds, int timeout) const override;
    int socket(int domain, int type, int protocol) const override;
    int connect(int sockfd, const struct sockaddr* addr,
                socklen_t addrlen) const override;
    ssize_t sendfile(int out_fd, int in_fd, off_t* offset,
                     size_t count) const override;
    int getaddrinfo(const char* node, const char* service,
                    const struct addrinfo* hints,
                    struct addrinfo** res) const override;
    void freeaddrinfo(struct addrinfo* res) const override;
    /* returns 0 on failure, or if the file is zero bytes. */
    std::int64_t getSize(const char* pathname) const override;
};

inline std::system_error errnoException(const std::string& message)
{
    return std::system_error(errno, std::generic_category(), message);
}

/** @brief Default instantiation of sys */
extern SysImpl sys_impl;

} // namespace internal
