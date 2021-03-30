#include "config.h"

#include "item_updater_helper.hpp"

#include <thread>

namespace phosphor
{
namespace software
{
namespace updater
{

void Helper::setEntry(const std::string& /* entryId */, uint8_t /* value */)
{
    // Empty
}

void Helper::clearEntry(const std::string& /* entryId */)
{
    // Empty
}

void Helper::cleanup()
{
    // Empty
}

void Helper::factoryReset()
{
    // Empty
}

void Helper::removeVersion(const std::string& versionId)
{
    auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    auto serviceFile = "obmc-flash-mmc-remove@" + versionId + ".service";
    method.append(serviceFile, "replace");
    bus.call_noreply(method);
}

void Helper::updateUbootVersionId(const std::string& versionId)
{
    auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    auto serviceFile = "obmc-flash-mmc-setprimary@" + versionId + ".service";
    method.append(serviceFile, "replace");
    bus.call_noreply(method);

    // Wait a few seconds for the service file to finish, otherwise the BMC may
    // be rebooted while pointing to a non-existent version.
    constexpr auto setPrimaryWait = std::chrono::seconds(3);
    std::this_thread::sleep_for(setPrimaryWait);
}

void Helper::mirrorAlt()
{
    // Empty
}

} // namespace updater
} // namespace software
} // namespace phosphor
