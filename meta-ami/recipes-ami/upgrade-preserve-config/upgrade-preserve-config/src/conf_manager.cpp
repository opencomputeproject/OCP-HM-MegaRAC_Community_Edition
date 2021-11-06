#include "config.h"
#include "conf_manager.hpp"
#include "preserve.hpp"
#include "preserve_serialize.hpp"

#include <experimental/filesystem>

namespace phosphor
{
namespace software
{
namespace preserve
{

ConfManager::ConfManager(sdbusplus::bus::bus& bus, const char* objPath) :
    dbusPersistentLocation(UPGRADE_CONF_PERSIST_PATH), bus(bus),
    objectPath(objPath)
{}

void ConfManager::restoreConfig()
{
    preserve = std::make_unique<PreserveConf>(bus, objectPath.c_str(), *this);

    if (!fs::exists(dbusPersistentLocation) ||
        fs::is_empty(dbusPersistentLocation))
    {
        return;
    }

    auto confFile = dbusPersistentLocation / UPGRADE_CONF_PERSIST_FILE;

    if (!fs::is_regular_file(confFile))
    {
        return;
    }

    deserialize(confFile, *preserve);
}


} // namespace preserve
} // namespace software
} // namespace phosphor
