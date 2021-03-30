#include <string>
#include <experimental/filesystem>
#include "config.h"
#include "ldap_mapper_mgr.hpp"

int main(int argc, char** argv)
{
    auto bus = sdbusplus::bus::new_default();
    sdbusplus::server::manager::manager objManager(
        bus, phosphor::user::mapperMgrRoot);

    phosphor::user::LDAPMapperMgr mapperMgr(bus, phosphor::user::mapperMgrRoot,
                                            LDAP_MAPPER_PERSIST_PATH);

    // Create a directory to persist errors.
    std::experimental::filesystem::create_directories(LDAP_MAPPER_PERSIST_PATH);

    // Restore the serialized LDAP group privilege mapping.
    mapperMgr.restore();

    // Claim the bus name for the application
    bus.request_name(LDAP_MAPPER_MANAGER_BUSNAME);

    // Wait for client request
    while (true)
    {
        // Process D-Bus calls
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
