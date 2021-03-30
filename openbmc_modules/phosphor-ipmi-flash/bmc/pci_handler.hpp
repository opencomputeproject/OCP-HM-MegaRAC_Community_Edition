#pragma once

#include "data_handler.hpp"
#include "internal/sys.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace ipmi_flash
{

/**
 * Data handler for reading and writing data via the P2A bridge.
 *
 * @note: Currently implemented to support only aspeed-p2a-ctrl.
 */
class PciDataHandler : public DataInterface
{
  public:
    PciDataHandler(std::uint32_t regionAddress, std::size_t regionSize,
                   const internal::Sys* sys = &internal::sys_impl) :
        regionAddress(regionAddress),
        memoryRegionSize(regionSize), sys(sys){};

    bool open() override;
    bool close() override;
    std::vector<std::uint8_t> copyFrom(std::uint32_t length) override;
    bool writeMeta(const std::vector<std::uint8_t>& configuration) override;
    std::vector<std::uint8_t> readMeta() override;

  private:
    std::uint32_t regionAddress;
    std::uint32_t memoryRegionSize;
    const internal::Sys* sys;

    int mappedFd = -1;
    std::uint8_t* mapped = nullptr;
    static const std::string p2aControlPath;
};

} // namespace ipmi_flash
