#pragma once
#include <cstddef>
#include <linux/gpio.h>

namespace gpioplus
{
namespace internal
{

/** @class Sys
 *  @brief Overridable direct syscall interface
 */
class Sys
{
  public:
    virtual ~Sys() = default;

    virtual int open(const char* pathname, int flags) const = 0;
    virtual int dup(int oldfd) const = 0;
    virtual int close(int fd) const = 0;
    virtual int read(int fd, void* buf, size_t count) const = 0;
    virtual int fcntl_setfl(int fd, int flags) const = 0;
    virtual int fcntl_getfl(int fd) const = 0;

    virtual int
        gpiohandle_get_line_values(int fd,
                                   struct gpiohandle_data* data) const = 0;
    virtual int
        gpiohandle_set_line_values(int fd,
                                   struct gpiohandle_data* data) const = 0;
    virtual int gpio_get_chipinfo(int fd, struct gpiochip_info* info) const = 0;
    virtual int gpio_get_lineinfo(int fd, struct gpioline_info* info) const = 0;
    virtual int
        gpio_get_linehandle(int fd,
                            struct gpiohandle_request* request) const = 0;
    virtual int gpio_get_lineevent(int fd,
                                   struct gpioevent_request* request) const = 0;
};

/** @class SysImpl
 *  @brief syscall concrete implementation
 *  @details Passes through all calls to the normal linux syscalls
 */
class SysImpl : public Sys
{
  public:
    int open(const char* pathname, int flags) const override;
    int dup(int oldfd) const override;
    int close(int fd) const override;
    int read(int fd, void* buf, size_t count) const override;
    int fcntl_setfl(int fd, int flags) const override;
    int fcntl_getfl(int fd) const override;

    int gpiohandle_get_line_values(int fd,
                                   struct gpiohandle_data* data) const override;
    int gpiohandle_set_line_values(int fd,
                                   struct gpiohandle_data* data) const override;
    int gpio_get_chipinfo(int fd, struct gpiochip_info* info) const override;
    int gpio_get_lineinfo(int fd, struct gpioline_info* info) const override;
    int gpio_get_linehandle(int fd,
                            struct gpiohandle_request* request) const override;
    int gpio_get_lineevent(int fd,
                           struct gpioevent_request* request) const override;
};

/** @brief Default instantiation of sys */
extern SysImpl sys_impl;

} // namespace internal
} // namespace gpioplus
