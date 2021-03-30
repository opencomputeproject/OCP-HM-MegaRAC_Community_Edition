#pragma once

#include "config.h"

#include "xyz/openbmc_project/Logging/ErrorBlocksTransition/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Association/Definitions/server.hpp>

namespace phosphor
{
namespace logging
{

using BlockIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Logging::server::ErrorBlocksTransition,
    sdbusplus::xyz::openbmc_project::Association::server::Definitions>;

using AssociationList =
    std::vector<std::tuple<std::string, std::string, std::string>>;

/** @class Block
 *  @brief OpenBMC logging Block implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Logging.ErrorBlocksTransition DBus API
 */
class Block : public BlockIface
{
  public:
    Block() = delete;
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;
    Block(Block&&) = delete;
    Block& operator=(Block&&) = delete;
    virtual ~Block() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] path - Path to attach at.
     *  @param[in] entryId - Distinct ID of the error.
     */
    Block(sdbusplus::bus::bus& bus, const std::string& path, uint32_t entryId) :
        BlockIface(bus, path.c_str()), entryId(entryId)
    {
        std::string entryPath{std::string(OBJ_ENTRY) + '/' +
                              std::to_string(entryId)};
        AssociationList assoc{std::make_tuple(std::string{"blocking_error"},
                                              std::string{"blocking_obj"},
                                              entryPath)};
        associations(std::move(assoc));
    };

    uint32_t entryId;

  private:
};

} // namespace logging
} // namespace phosphor
