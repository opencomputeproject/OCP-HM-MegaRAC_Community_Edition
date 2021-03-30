#include <fcntl.h>
#include <gpioplus/internal/sys.hpp>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace gpioplus
{
namespace internal
{

int SysImpl::open(const char* pathname, int flags) const
{
    return ::open(pathname, flags);
}

int SysImpl::dup(int oldfd) const
{
    return ::dup(oldfd);
}

int SysImpl::close(int fd) const
{
    return ::close(fd);
}

int SysImpl::read(int fd, void* buf, size_t count) const
{
    return ::read(fd, buf, count);
}

int SysImpl::fcntl_setfl(int fd, int flags) const
{
    return ::fcntl(fd, F_SETFL, flags);
}

int SysImpl::fcntl_getfl(int fd) const
{
    return ::fcntl(fd, F_GETFL);
}

int SysImpl::gpiohandle_get_line_values(int fd,
                                        struct gpiohandle_data* data) const
{
    return ::ioctl(fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, data);
}

int SysImpl::gpiohandle_set_line_values(int fd,
                                        struct gpiohandle_data* data) const
{
    return ::ioctl(fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, data);
}

int SysImpl::gpio_get_chipinfo(int fd, struct gpiochip_info* info) const
{
    return ::ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, info);
}

int SysImpl::gpio_get_lineinfo(int fd, struct gpioline_info* info) const
{
    return ioctl(fd, GPIO_GET_LINEINFO_IOCTL, info);
}

int SysImpl::gpio_get_linehandle(int fd,
                                 struct gpiohandle_request* request) const
{
    return ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, request);
}

int SysImpl::gpio_get_lineevent(int fd, struct gpioevent_request* request) const
{
    return ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, request);
}

SysImpl sys_impl;

} // namespace internal
} // namespace gpioplus
