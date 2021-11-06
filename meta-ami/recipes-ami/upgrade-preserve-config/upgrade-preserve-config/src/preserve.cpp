#include "config.h"

#include "conf_manager.hpp"
#include "preserve.hpp"
#include "preserve_serialize.hpp"

#include <fstream>
#include <vector>

namespace phosphor
{
namespace software
{
namespace preserve
{

std::vector<std::string> ipmiPreserveFiles = {
    "/var/lib/phosphor-settings-manager"
};

std::vector<std::string> userPreserveFiles = {
    "/etc/group",
    "/etc/gshadow",
    "/etc/passwd",
    "/etc/shadow",
    "/var/lib/ipmi", /* ipmi user permission data */
    "/etc/ipmi_pass"
};

std::vector<std::string> hostnamePreserveFiles = {
    "/etc/hostname",
    "/etc/machine-info"
};

std::vector<std::string> ldapPreserveFiles = {
    "/var/lib/phosphor-ldap-conf",
    "/var/lib/phosphor-ldap-mapper",
    "/etc/nslcd",
    "/etc/nslcd.conf"
};

std::vector<std::string> certPreserveFiles = {
    "/etc/ssl/certs"
};

std::vector<std::string> sdrPreserveFiles = {
    "/var/configuration"
};

std::vector<std::string> selPreserveFiles = {
    "/var/sellog",
    "/var/lib/phosphor-logging"
};

std::vector<std::string> networkPreserveFiles = {
    "/etc/systemd/network",
    "/etc/resolv.conf"
};

void updateWhitelist(std::vector<std::string> &lines, bool add)
{
    std::string line;
    std::ifstream file;
    std::ofstream temp;

    file.open(WHITELIST_FILE);
    temp.open(std::string(std::string(WHITELIST_FILE) + std::string(".temp")).c_str());

    while (getline(file, line))
    {
        if (std::find(lines.begin(), lines.end(), line) == lines.end())
        {
            temp << line << std::endl;
        }
    }

    if (add)
    {
        for (std::string l : lines)
        {
            temp << l << std::endl;
        }
    }

    file.close();
    temp.close();

    remove(WHITELIST_FILE);
    rename(std::string(std::string(WHITELIST_FILE) + std::string(".temp")).c_str(), WHITELIST_FILE);

}

PreserveConf::PreserveConf(sdbusplus::bus::bus& bus, const char *objPath,
        ConfManager& parent) :
        Ifaces(bus, objPath, true),
        parent(parent)
{
    /* sync whitelist to defaults before deserialization */
    updateWhitelist(ipmiPreserveFiles, Ifaces::iPMI());
    updateWhitelist(userPreserveFiles, Ifaces::user());
    updateWhitelist(ldapPreserveFiles, Ifaces::lDAP());
    updateWhitelist(certPreserveFiles, Ifaces::certificates());
    updateWhitelist(hostnamePreserveFiles, Ifaces::hostname());
    updateWhitelist(sdrPreserveFiles, Ifaces::sDR());
    updateWhitelist(selPreserveFiles, Ifaces::sEL());
    updateWhitelist(networkPreserveFiles, Ifaces::network());
}

bool PreserveConf::iPMI(bool value)
{
    if (value == Ifaces::iPMI())
    {
        return value;
    }

    auto ipmi = Ifaces::iPMI(value);
    serialize(*this, parent.dbusPersistentLocation);

    updateWhitelist(ipmiPreserveFiles, ipmi);

    return ipmi;
}

bool PreserveConf::user(bool value)
{
    if (value == Ifaces::user())
    {
        return value;
    }

    auto user = Ifaces::user(value);
    serialize(*this, parent.dbusPersistentLocation);

    updateWhitelist(userPreserveFiles, user);

    return user;
}

bool PreserveConf::lDAP(bool value)
{
    if (value == Ifaces::lDAP())
    {
        return value;
    }

    auto ldap = Ifaces::lDAP(value);
    serialize(*this, parent.dbusPersistentLocation);

    updateWhitelist(ldapPreserveFiles, ldap);

    return ldap;
}

bool PreserveConf::certificates(bool value)
{
    if (value == Ifaces::certificates())
    {
        return value;
    }

    auto certs = Ifaces::certificates(value);
    serialize(*this, parent.dbusPersistentLocation);

    updateWhitelist(certPreserveFiles, certs);

    return certs;
}

bool PreserveConf::hostname(bool value)
{
    if (value == Ifaces::hostname())
    {
        return value;
    }

    auto hostname = Ifaces::hostname(value);
    serialize(*this, parent.dbusPersistentLocation);

    updateWhitelist(hostnamePreserveFiles, hostname);

    return hostname;
}

bool PreserveConf::sEL(bool value)
{
    if (value == Ifaces::sEL())
    {
        return value;
    }

    auto sel = Ifaces::sEL(value);
    serialize(*this, parent.dbusPersistentLocation);

    updateWhitelist(selPreserveFiles, sel);

    return sel;
}

bool PreserveConf::sDR(bool value)
{
    if (value == Ifaces::sDR())
    {
        return value;
    }

    auto sdr = Ifaces::sDR(value);
    serialize(*this, parent.dbusPersistentLocation);

    updateWhitelist(sdrPreserveFiles, sdr);

    return sdr;
}

bool PreserveConf::network(bool value)
{
    if (value == Ifaces::network())
    {
        return value;
    }

    auto net = Ifaces::network(value);
    serialize(*this, parent.dbusPersistentLocation);

    updateWhitelist(networkPreserveFiles, net);

    return net;
}

} // namespace preserve
} // namespace software
} // namespace phosphor
