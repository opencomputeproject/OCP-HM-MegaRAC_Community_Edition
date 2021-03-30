#pragma once

#include "internal/sys.hpp"
#include "window_hw_interface.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace ipmi_flash
{

class LpcMapperNuvoton : public HardwareMapperInterface
{
  public:
    static std::unique_ptr<HardwareMapperInterface>
        createNuvotonMapper(std::uint32_t regionAddress,
                            std::uint32_t regionSize);

    /**
     * Create an LpcMapper for Nuvoton.
     *
     * @param[in] regionAddress - where to map the window into BMC memory.
     * @param[in] regionSize - the size to map for copying data.
     * @param[in] a sys call interface pointer.
     * @todo Needs reserved memory region's physical address and size.
     */
    LpcMapperNuvoton(std::uint32_t regionAddress, std::uint32_t regionSize,
                     const internal::Sys* sys = &internal::sys_impl) :
        regionAddress(regionAddress),
        memoryRegionSize(regionSize), sys(sys){};

    /** Attempt to map the window for copying bytes, after mapWindow is called.
     * throws MapperException
     */
    MemorySet open() override;

    void close() override;

    WindowMapResult mapWindow(std::uint32_t address,
                              std::uint32_t length) override;

  private:
    std::uint32_t regionAddress;
    std::uint32_t memoryRegionSize;
    const internal::Sys* sys;

    /* The file handle to /dev/mem. */
    int mappedFd = -1;

    /* The pointer to the memory-mapped region. */
    std::uint8_t* mapped = nullptr;
};

} // namespace ipmi_flash
