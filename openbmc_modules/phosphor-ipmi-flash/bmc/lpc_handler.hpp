#pragma once

#include "data_handler.hpp"
#include "mapper_errors.hpp"
#include "window_hw_interface.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace ipmi_flash
{

struct LpcRegion
{
    /* Host LPC address where the chunk is to be mapped. */
    std::uint32_t address;

    /* Size of the chunk to be mapped. */
    std::uint32_t length;
} __attribute__((packed));

/**
 * Data Handler for configuration the ASPEED LPC memory region, reading and
 * writing data.
 */
class LpcDataHandler : public DataInterface
{
  public:
    /**
     * Create an LpcDataHandler.
     *
     * @param[in] mapper - pointer to a mapper implementation to use.
     */
    explicit LpcDataHandler(std::unique_ptr<HardwareMapperInterface> mapper) :
        mapper(std::move(mapper)), initialized(false)
    {}

    bool open() override;
    bool close() override;
    std::vector<std::uint8_t> copyFrom(std::uint32_t length) override;
    bool writeMeta(const std::vector<std::uint8_t>& configuration) override;
    std::vector<std::uint8_t> readMeta() override;

  private:
    bool setInitializedAndReturn(bool value)
    {
        if (value)
        {
            try
            {
                /* Try really opening the map. */
                memory = mapper->open();
            }
            catch (const MapperException& e)
            {
                std::fprintf(stderr, "received mapper exception: %s\n",
                             e.what());
                return false;
            }
        }

        initialized = value;
        return value;
    }

    std::unique_ptr<HardwareMapperInterface> mapper;
    bool initialized;
    /* The LPC Handler does not take ownership of this, in case there's cleanup
     * required for close()
     */
    MemorySet memory = {};

    /* Offset in reserved memory at which host data arrives. */
    /* Size of the chunk of the memory region in use by the host (e.g.
     * mapped over external block mechanism).
     */
    WindowMapResult mappingResult = {};
};

} // namespace ipmi_flash
