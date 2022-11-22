/******************************************************************************
 *
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with no express
 * or implied warranties, other than those that are expressly stated in the
 * License.
 *
 ******************************************************************************/

#include "utils_dbusplus.hpp"
extern "C" {
#include "CrashdumpSections/MetaData.h"
#include "CrashdumpSections/utils.h"
}

using PropertyValue =
    std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
                 uint64_t, double, std::string>;

namespace crashdump
{
int getBMCVersionDBus(char* bmcVerStr, size_t bmcVerStrSize)
{
    using ManagedObjectType = boost::container::flat_map<
        sdbusplus::message::object_path,
        boost::container::flat_map<
            std::string, boost::container::flat_map<
                             std::string, std::variant<std::string>>>>;

    if (bmcVerStr == nullptr)
    {
        return ACD_FAILURE;
    }

    sdbusplus::bus::bus dbus = sdbusplus::bus::new_default_system();
    sdbusplus::message::message getObjects = dbus.new_method_call(
        "xyz.openbmc_project.Software.BMC.Updater",
        "/xyz/openbmc_project/software", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
    ManagedObjectType bmcUpdaterIntfs;
    try
    {
        sdbusplus::message::message resp = dbus.call(getObjects);
        resp.read(bmcUpdaterIntfs);
    }
    catch (sdbusplus::exception_t& e)
    {
        return ACD_FAILURE;
    }

    for (const std::pair<
             sdbusplus::message::object_path,
             boost::container::flat_map<
                 std::string, boost::container::flat_map<
                                  std::string, std::variant<std::string>>>>&
             pathPair : bmcUpdaterIntfs)
    {
        boost::container::flat_map<
            std::string,
            boost::container::flat_map<
                std::string, std::variant<std::string>>>::const_iterator
            softwareVerIt =
                pathPair.second.find("xyz.openbmc_project.Software.Version");
        if (softwareVerIt != pathPair.second.end())
        {
            boost::container::flat_map<std::string, std::variant<std::string>>::
                const_iterator purposeIt =
                    softwareVerIt->second.find("Purpose");
            if (purposeIt != softwareVerIt->second.end())
            {
                const std::string* bmcPurpose =
                    std::get_if<std::string>(&purposeIt->second);
                if (bmcPurpose->compare(
                        "xyz.openbmc_project.Software.Version.BMC"))
                {
                    boost::container::flat_map<
                        std::string, std::variant<std::string>>::const_iterator
                        versionIt = softwareVerIt->second.find("Version");
                    if (versionIt != softwareVerIt->second.end())
                    {
                        const std::string* bmcVersion =
                            std::get_if<std::string>(&versionIt->second);
                        if (bmcVersion != nullptr)
                        {
                            size_t copySize =
                                std::min(bmcVersion->size(), bmcVerStrSize - 1);
                            bmcVersion->copy(bmcVerStr, copySize);
                            return ACD_SUCCESS;
                        }
                    }
                }
            }
        }
    }
    return ACD_FAILURE;
}

int getBIOSVersionDBus(char* biosVerStr, size_t biosVerStrSize)
{
    PropertyValue biosSoftwareVersion{};
    if (biosVerStr == nullptr)
    {
        return ACD_FAILURE;
    }
    sdbusplus::bus::bus dbus = sdbusplus::bus::new_default_system();
    auto method =
        dbus.new_method_call("xyz.openbmc_project.Settings",
                             "/xyz/openbmc_project/software/bios_active",
                             "org.freedesktop.DBus.Properties", "Get");
    method.append("xyz.openbmc_project.Software.Version", "Version");
    try
    {
        auto reply = dbus.call(method);
        reply.read(biosSoftwareVersion);
    }
    catch (sdbusplus::exception_t& e)
    {
        return ACD_FAILURE;
    }
    std::string* biosVersion = &(std::get<std::string>(biosSoftwareVersion));
    if (biosVersion != nullptr)
    {
        size_t copySize = std::min(biosVersion->size(), biosVerStrSize - 1);
        biosVersion->copy(biosVerStr, copySize);
        return ACD_SUCCESS;
    }
    return ACD_FAILURE;
}

std::shared_ptr<sdbusplus::bus::match::match>
    startHostStateMonitor(std::shared_ptr<sdbusplus::asio::connection> conn)
{
    return std::make_shared<sdbusplus::bus::match::match>(
        *conn,
        "type='signal',interface='org.freedesktop.DBus.Properties',arg0='xyz."
        "openbmc_project.State.Host'",
        [](sdbusplus::message::message& msg) {
            std::string thresholdInterface;
            boost::container::flat_map<std::string, std::variant<std::string>>
                propertiesChanged;

            msg.read(thresholdInterface, propertiesChanged);

            if (propertiesChanged.empty())
            {
                return;
            }

            std::string event = propertiesChanged.begin()->first;

            auto variant =
                std::get_if<std::string>(&propertiesChanged.begin()->second);

            if (event.empty() || nullptr == variant)
            {
                return;
            }

            if (event == "CurrentHostState")
            {
                if (*variant == "xyz.openbmc_project.State.Host.HostState.Off")
                {
                    setResetDetected();
                    return;
                }
            }
        });
}

} // namespace crashdump

/******************************************************************************
 *
 *   fillBmcVersionJson
 *
 *   This function fills in the bmc_fw_ver JSON info
 *
 ******************************************************************************/
int fillBmcVersion(char* cSectionName, cJSON* pJsonChild)
{
    char bmcVersion[SI_BMC_VER_LEN] = {0};
    crashdump::getBMCVersionDBus(bmcVersion, sizeof(bmcVersion));
    // Fill in BMC Version string
    cJSON_AddStringToObject(pJsonChild, cSectionName, bmcVersion);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillBiosIdJson
 *
 *   This function fills in the bios_id JSON info
 *
 ******************************************************************************/
int fillBiosId(char* cSectionName, cJSON* pJsonChild)
{
    char biosVersion[SI_BIOS_ID_LEN] = {0};
    crashdump::getBIOSVersionDBus(biosVersion, sizeof(biosVersion));
    // Fill in BIOS Version string
    cJSON_AddStringToObject(pJsonChild, cSectionName, biosVersion);
    return ACD_SUCCESS;
}

static SSysInfoCommonSection sSysInfoCommonTable[] = {
    {"me_fw_ver", fillMeVersion},
    {"bmc_fw_ver", fillBmcVersion},
    {"bios_id", fillBiosId},
    {"crashdump_ver", fillCrashdumpVersion},
};

/******************************************************************************
 *
 *   sysInfoCommonJson
 *
 *   This function formats the system information into a JSON object
 *
 ******************************************************************************/
static int sysInfoCommonJson(cJSON* pJsonChild)
{
    uint32_t i = 0;
    int r = 0;
    int ret = 0;

    for (i = 0;
         i < (sizeof(sSysInfoCommonTable) / sizeof(SSysInfoCommonSection)); i++)
    {
        r = sSysInfoCommonTable[i].FillSysInfoJson(
            sSysInfoCommonTable[i].sectionName, pJsonChild);
        if (r == 1)
        {
            ret = 1;
        }
    }
    return ret;
}
/******************************************************************************
 *
 *   logSysInfoCommon
 *
 *   This function gathers various bits of system information and compiles them
 *   into a single group
 *
 ******************************************************************************/
int logSysInfoCommon(cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    return commonMetaDataEnabled ? sysInfoCommonJson(pJsonChild) : 0;
}
