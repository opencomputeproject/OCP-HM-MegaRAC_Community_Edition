#pragma once

#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace phosphor
{
namespace dump
{

class Manager;
namespace internal
{

using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create>;
using Mgr = phosphor::dump::Manager;

/** @class Manager
 *  @brief Implementation for the
 *         xyz.openbmc_project.Dump.Internal.Create DBus API.
 */
class Manager : public CreateIface
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] dumpMgr - Dump Manager object
     *  @param[in] path - Path to attach at.
     */
    Manager(sdbusplus::bus::bus& bus, Mgr& dumpMgr, const char* path) :
        CreateIface(bus, path), dumpMgr(dumpMgr){};

    /**  @brief Implementation for Create
     *  Create BMC Dump based on the Dump type.
     *
     *  @param[in] type - Type of the Dump.
     *  @param[in] fullPaths - List of absolute paths to the files
     *             to be included as part of Dump package.
     */
    void create(Type type, std::vector<std::string> fullPaths) override;

  private:
    /**  @brief Dump Manager object. */
    Mgr& dumpMgr;
};

} // namespace internal
} // namespace dump
} // namespace phosphor
