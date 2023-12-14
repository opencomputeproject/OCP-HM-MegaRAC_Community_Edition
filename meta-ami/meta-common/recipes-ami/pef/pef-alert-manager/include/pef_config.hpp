#pragma once
#include <boost/asio/io_service.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/message.hpp>

using Json = nlohmann::json;

static constexpr const char* pefBus = "xyz.openbmc_project.pef.alert.manager";
static constexpr const char* pefObj = "/xyz/openbmc_project/PefAlertManager";
static constexpr const char* pefDbusIntf =
    "xyz.openbmc_project.pef.configurations";
static constexpr const char* pefConfInfoIntf =
    "xyz.openbmc_project.pef.PEFConfInfo";
static constexpr const char* pefArmPostponeTmrObj =
    "/xyz/openbmc_project/PefAlertManager/ArmPostponeTimer";
static constexpr const char* pefPostponeTmrIntf =
    "xyz.openbmc_project.pef.PEFPostponeTimer";
static constexpr const char* pefCountdownTmrIntf =
    "xyz.openbmc_project.pef.CountdownTmr";
static constexpr const char* systemGUIDIntf =
    "xyz.openbmc_project.pef.SystemGUID";
static constexpr const char* oemParamIntf = "xyz.openbmc_project.pef.OEMParam";
static constexpr const char* eventFilterTableObj =
    "/xyz/openbmc_project/PefAlertManager/EventFilterTable/Entry";
static constexpr const char* eventFilterTableIntf =
    "xyz.openbmc_project.pef.EventFilterTable";
static constexpr const char* alertPolicyTableObj =
    "/xyz/openbmc_project/PefAlertManager/AlertPolicyTable/Entry";
static constexpr const char* alertPolicyTableIntf =
    "xyz.openbmc_project.pef.AlertPolicyTable";
static constexpr const char* alertStringTableObj =
    "/xyz/openbmc_project/PefAlertManager/AlertStringTable/Entry";
static constexpr const char* alertStringTableIntf =
    "xyz.openbmc_project.pef.AlertStringTable";

// static constexpr const char* pefConfFilePath =
// "/usr/share/pef-alert-manager/pef-alert-manager.json";
static constexpr const char* pefConfFilePath =
    "/var/lib/pef-alert-manager/pef-alert-manager.json";

/**
 *parseJSONConfig -Parse out JSON config file
 */
Json parseJSONConfig(const std::string& configFile);

/**
 *parsePefConfToDbus - Parse all configuration data from json and create dbus
 *property for json entries.
 **/
void parsePefConfToDbus(std::shared_ptr<sdbusplus::asio::connection> conn,
                        sdbusplus::asio::object_server& objectServer);
