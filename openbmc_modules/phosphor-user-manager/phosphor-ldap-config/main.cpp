#include "config.h"
#include "ldap_config_mgr.hpp"
#include <experimental/filesystem>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

int main(int argc, char* argv[])
{
    using namespace phosphor::logging;
    using namespace sdbusplus::xyz::openbmc_project::Common::Error;
    namespace fs = std::experimental::filesystem;
    fs::path configDir = fs::path(LDAP_CONFIG_FILE).parent_path();

    if (!fs::exists(configDir / phosphor::ldap::defaultNslcdFile) ||
        !fs::exists(configDir / phosphor::ldap::nsSwitchFile))
    {
        log<level::ERR>("Error starting LDAP Config App, configfile(s) are "
                        "missing, exiting!!!");
        elog<InternalFailure>();
    }
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager for the 'root' path of the LDAP config.
    sdbusplus::server::manager::manager objManager(bus, LDAP_CONFIG_ROOT);

    phosphor::ldap::ConfigMgr mgr(bus, LDAP_CONFIG_ROOT, LDAP_CONFIG_FILE,
                                  LDAP_CONF_PERSIST_PATH, TLS_CACERT_PATH,
                                  TLS_CERT_FILE);
    mgr.restore();

    bus.request_name(LDAP_CONFIG_BUSNAME);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
