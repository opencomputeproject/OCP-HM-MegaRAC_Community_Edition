#include "io.hpp"

#include "internal/sys.hpp"

#include <fcntl.h>

#include <cstdint>
#include <cstring>
#include <string>

namespace host_tool
{

const std::string DevMemDevice::devMemPath = "/dev/mem";

bool DevMemDevice::read(const std::size_t offset, const std::size_t length,
                        void* const destination)
{
    devMemFd = sys->open(devMemPath.c_str(), O_RDONLY);
    if (devMemFd < 0)
    {
        return false;
    }

    /* Map based on aligned addresses - behind the scenes. */
    const std::size_t alignedDiff = offset % sys->getpagesize();
    const std::size_t alignedOffset = offset - alignedDiff;
    const std::size_t alignedSize = length + alignedDiff;

    // addr, length, prot, flags, fd, offset
    devMemMapped = sys->mmap(0, alignedSize, PROT_READ, MAP_SHARED, devMemFd,
                             alignedOffset);
    if (devMemMapped == MAP_FAILED)
    {
        std::fprintf(stderr, "Failed to mmap at offset: 0x%zx, length: %zu\n",
                     offset, length);
        sys->close(devMemFd);
        return false;
    }

    void* alignedSource =
        static_cast<std::uint8_t*>(devMemMapped) + alignedDiff;

    /* Copy the bytes. */
    std::memcpy(destination, alignedSource, length);

    /* Close the map between reads for now. */
    sys->munmap(devMemMapped, length);
    sys->close(devMemFd);

    return true;
}

bool DevMemDevice::write(const std::size_t offset, const std::size_t length,
                         const void* const source)
{
    devMemFd = sys->open(devMemPath.c_str(), O_RDWR);
    if (devMemFd < 0)
    {
        std::fprintf(stderr, "Failed to open /dev/mem for writing\n");
        return false;
    }

    /* Map based on aligned addresses - behind the scenes. */
    const std::size_t alignedDiff = offset % sys->getpagesize();
    const std::size_t alignedOffset = offset - alignedDiff;
    const std::size_t alignedSize = length + alignedDiff;

    // addr, length, prot, flags, fd, offset
    devMemMapped = sys->mmap(0, alignedSize, PROT_WRITE, MAP_SHARED, devMemFd,
                             alignedOffset);

    if (devMemMapped == MAP_FAILED)
    {
        std::fprintf(stderr, "Failed to mmap at offset: 0x%zx, length: %zu\n",
                     offset, length);
        sys->close(devMemFd);
        return false;
    }

    void* alignedDestination =
        static_cast<std::uint8_t*>(devMemMapped) + alignedDiff;

    /* Copy the bytes. */
    std::memcpy(alignedDestination, source, length);

    /* Close the map between writes for now. */
    sys->munmap(devMemMapped, length);
    sys->close(devMemFd);

    return true;
}

PpcMemDevice::~PpcMemDevice()
{
    // Attempt to close in case reads or writes didn't close themselves
    if (ppcMemFd >= 0)
    {
        sys->close(ppcMemFd);
    }
}

bool PpcMemDevice::read(const std::size_t offset, const std::size_t length,
                        void* const destination)
{
    ppcMemFd = sys->open(ppcMemPath.c_str(), O_RDWR);
    if (ppcMemFd < 0)
    {
        std::fprintf(stderr, "Failed to open PPC LPC access path: %s",
                     ppcMemPath.c_str());
        return false;
    }

    int ret = sys->pread(ppcMemFd, destination, length, offset);
    if (ret < 0)
    {
        std::fprintf(stderr, "IO read failed at offset: 0x%zx, length: %zu\n",
                     offset, length);
        sys->close(ppcMemFd);
        return false;
    }

    sys->close(ppcMemFd);
    return true;
}

bool PpcMemDevice::write(const std::size_t offset, const std::size_t length,
                         const void* const source)
{
    ppcMemFd = sys->open(ppcMemPath.c_str(), O_RDWR);
    if (ppcMemFd < 0)
    {
        std::fprintf(stderr, "Failed to open PPC LPC access path: %s",
                     ppcMemPath.c_str());
        return false;
    }

    ssize_t ret = sys->pwrite(ppcMemFd, source, length, offset);
    if (ret < 0)
    {
        std::fprintf(stderr, "IO write failed at offset: 0x%zx, length: %zu\n",
                     offset, length);
        sys->close(ppcMemFd);
        return false;
    }

    sys->close(ppcMemFd);
    return true;
}

} // namespace host_tool
