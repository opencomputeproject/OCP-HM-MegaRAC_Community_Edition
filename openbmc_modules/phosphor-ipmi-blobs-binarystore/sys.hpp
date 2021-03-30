#pragma once

#include <unistd.h>

namespace binstore
{

namespace internal
{

/** @class Sys
 *  @brief Overridable direct syscall interface
 *
 *  TODO: factor this out into a syscall class shared by all upstream projects
 */
class Sys
{
  public:
    virtual ~Sys() = default;
    virtual int open(const char* pathname, int flags) const = 0;
    virtual int close(int fd) const = 0;
    virtual off_t lseek(int fd, off_t offset, int whence) const = 0;
    virtual ssize_t read(int fd, void* buf, size_t count) const = 0;
    virtual ssize_t write(int fd, const void* buf, size_t count) const = 0;
};

/** @class SysImpl
 *  @brief syscall concrete implementation
 *  @details Passes through all calls to the normal linux syscalls
 */
class SysImpl : public Sys
{
  public:
    int open(const char* pathname, int flags) const override;
    int close(int fd) const override;
    off_t lseek(int fd, off_t offset, int whence) const override;
    ssize_t read(int fd, void* buf, size_t count) const override;
    ssize_t write(int fd, const void* buf, size_t count) const override;
};

/** @brief Default instantiation of sys */
extern SysImpl sys_impl;

} // namespace internal

} // namespace binstore
