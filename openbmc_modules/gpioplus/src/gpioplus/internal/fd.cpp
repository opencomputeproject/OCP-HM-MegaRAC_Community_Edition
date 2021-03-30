#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <gpioplus/internal/fd.hpp>
#include <system_error>
#include <utility>

namespace gpioplus
{
namespace internal
{

Fd::Fd(const char* pathname, int flags, const Sys* sys) :
    sys(sys), fd(sys->open(pathname, flags))
{
    if (fd < 0)
    {
        throw std::system_error(errno, std::generic_category(), "Opening FD");
    }
}

static int dup(int oldfd, const Sys* sys)
{
    int fd = sys->dup(oldfd);
    if (fd < 0)
    {
        throw std::system_error(errno, std::generic_category(), "Duping FD");
    }
    return fd;
}

Fd::Fd(int fd, const Sys* sys) : sys(sys), fd(dup(fd, sys))
{
}

Fd::Fd(int fd, std::false_type, const Sys* sys) : sys(sys), fd(fd)
{
}

Fd::~Fd()
{
    try
    {
        reset();
    }
    catch (...)
    {
        std::abort();
    }
}

Fd::Fd(const Fd& other) : sys(other.sys), fd(dup(other.fd, sys))
{
}

Fd& Fd::operator=(const Fd& other)
{
    if (this != &other)
    {
        reset();
        sys = other.sys;
        fd = dup(other.fd, sys);
    }
    return *this;
}

Fd::Fd(Fd&& other) : sys(other.sys), fd(std::move(other.fd))
{
    other.fd = -1;
}

Fd& Fd::operator=(Fd&& other)
{
    if (this != &other)
    {
        reset();
        sys = other.sys;
        fd = std::move(other.fd);
        other.fd = -1;
    }
    return *this;
}

int Fd::operator*() const
{
    return fd;
}

const Sys* Fd::getSys() const
{
    return sys;
}

void Fd::setBlocking(bool enabled) const
{
    if (enabled)
    {
        setFlags(getFlags() & ~O_NONBLOCK);
    }
    else
    {
        setFlags(getFlags() | O_NONBLOCK);
    }
}

void Fd::setFlags(int flags) const
{
    int r = sys->fcntl_setfl(fd, flags);
    if (r == -1)
    {
        throw std::system_error(errno, std::generic_category(), "fcntl_setfl");
    }
}

int Fd::getFlags() const
{
    int flags = sys->fcntl_getfl(fd);
    if (flags == -1)
    {
        throw std::system_error(errno, std::generic_category(), "fcntl_getfl");
    }
    return flags;
}

void Fd::reset()
{
    if (fd < 0)
    {
        return;
    }

    int ret = sys->close(fd);
    fd = -1;
    if (ret != 0)
    {
        throw std::system_error(errno, std::generic_category(), "Closing FD");
    }
}

} // namespace internal
} // namespace gpioplus
