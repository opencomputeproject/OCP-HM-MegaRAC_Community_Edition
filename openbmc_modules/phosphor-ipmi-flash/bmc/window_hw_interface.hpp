#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace ipmi_flash
{

struct MemorySet
{
    int mappedFd = -1;
    std::uint8_t* mapped = nullptr;
};

/** The result from the mapWindow command. */
struct WindowMapResult
{
    /* The response can validly be 0, or EFBIG.  If it's EFBIG that means the
     * region available is within the requested region. If the value is anything
     * else, it's a complete failure.
     */
    std::uint8_t response;
    std::uint32_t windowOffset;
    std::uint32_t windowSize;
};

/**
 * Different LPC (or P2a) memory map implementations may require different
 * mechanisms for specific tasks such as mapping the memory window or copying
 * out data.
 */
class HardwareMapperInterface
{
  public:
    virtual ~HardwareMapperInterface() = default;

    /**
     * Open the driver or whatever and map the region.
     */
    virtual MemorySet open() = 0;

    /**
     * Close the mapper.  This could mean, send an ioctl to turn off the region,
     * or unmap anything mmapped.
     */
    virtual void close() = 0;

    /**
     * Returns a windowOffset and windowSize if the requested window was mapped.
     *
     * TODO: If the length requested is too large, windowSize will be written
     * with the max size that the BMC can map and returns false.
     *
     * @param[in] address - The address for mapping (passed to LPC window)
     * @param[in] length - The length of the region
     * @return WindowMapResult - the result of the call
     */
    virtual WindowMapResult mapWindow(std::uint32_t address,
                                      std::uint32_t length) = 0;
};

} // namespace ipmi_flash
