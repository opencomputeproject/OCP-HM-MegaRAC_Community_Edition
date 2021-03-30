/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <boost/algorithm/string/join.hpp>
#include <me_to_redfish_hooks.hpp>
#include <phosphor-logging/log.hpp>

#include <string_view>

namespace intel_oem::ipmi::sel::redfish_hooks::me
{
namespace health_event
{
namespace smbus_failure
{

static bool messageHook(const SELData& selData, std::string& eventId,
                        std::vector<std::string>& args)
{
    static const boost::container::flat_map<uint8_t, std::string> smlink = {
        {0x01, "SmLink0/0B"},
        {0x02, "SmLink1"},
        {0x03, "SmLink2"},
        {0x04, "SmLink3"},
        {0x05, "SmLink4"}};

    const auto errorDetails = selData.eventData3;
    const auto faultySmlink = smlink.find(selData.eventData2);
    if (faultySmlink == smlink.end())
    {
        return false;
    }

    eventId = "MESmbusLinkFailure";

    args.push_back(faultySmlink->second);
    args.push_back(utils::toHex(errorDetails));

    return true;
}
} // namespace smbus_failure

namespace fw_status
{
static const boost::container::flat_map<uint8_t, std::string>
    manufacturingError = {
        {0x00, "Generic error"},
        {0x01, "Wrong or missing VSCC table"},
        {0x02, "Wrong sensor scanning period in PIA"},
        {0x03, "Wrong device definition in PIA"},
        {0x04, "Reserved (Wrong SMART/CLST configuration)"},
        {0x05, "Intel ME FW configuration is inconsistent or out of range"},
        {0x06, "Reserved"},
        {0x07, "Intel ME FW configuration is corrupted"},
        {0x08, "SMLink0/0B misconfiguration"}};

static const boost::container::flat_map<uint8_t, std::string> peciOverDmiError =
    {{0x01, "DRAM Init Done HECI message not received by Intel ME before EOP"},
     {0x02, "System PCIe bus configuration not known or not valid on DID HECI "
            "message arrival to Intel ME"},
     {0x03, "PECI over DMI run-time failure"}};

static const boost::container::flat_map<uint8_t, std::string>
    mctpInterfaceError = {
        {0x01, "No DID HECI message received before EOP"},
        {0x02, "No MCTP_SET_BUS_OWNER HECI message received by Intel ME on EOP "
               "arrival "
               "to ME while MCTP stack is configured in Bus Owner Proxy mode"}};

static const boost::container::flat_map<uint8_t, std::string>
    unsupportedFeature = {{0x00, "Other Segment Defined Feature"},
                          {0x01, "Fast NM limiting"},
                          {0x02, "Volumetric Airflow and Outlet Temperature"},
                          {0x03, "CUPS"},
                          {0x04, "Thermal policies and Inlet Temperature"},
                          {0x05, "Platform limiting with MICs"},
                          {0x07, "Shared power supplies"},
                          {0x08, "MIC Proxy"},
                          {0x09, "Reset warning"},
                          {0x0A, "PMBus Proxy"},
                          {0x0B, "Always on"},
                          {0x0C, "IPMI Intel ME FW update"},
                          {0x0D, "MCTP bus owner"},
                          {0x0E, "MCTP bus owner proxy"},
                          {0x0F, "Dual BIOS"},
                          {0x10, "Battery less"}};

static const boost::container::flat_map<uint8_t, std::string> umaError = {
    {0x00, "UMA Read integrity error. Checksum of data read from UMA differs "
           "from expected one."},
    {0x01, "UMA Read/Write timeout. Timeout occurred during copying data "
           "from/to UMA."},
    {0x02,
     "UMA not granted. BIOS did not grant any UMA or DRAM INIT done message "
     "was not received from BIOS before EOP. Intel ME FW goes to recovery."},
    {0x03, "UMA size granted by BIOS differs from requested. ME FW goes to "
           "recovery."}};

static const boost::container::flat_map<uint8_t, std::string> pttHealthEvent = {
    {0x00, "Intel PTT disabled (PTT region is not present)."},
    {0x01, "Intel PTT downgrade (PTT data should be not available)."},
    {0x02, "Intel PTT disabled (battery less configuration)."}};

static const boost::container::flat_map<uint8_t, std::string>
    bootGuardHealthEvent = {
        {0x00, "Boot Guard flow error (possible reasons: verification timeout; "
               "verification error; BIOS Protection error)."}};

static const boost::container::flat_map<uint8_t, std::string> restrictedMode = {
    {0x01, "Firmware entered restricted mode – UMA is not available. "
           "Restricted features set."},
    {0x02, "Firmware exited restricted mode."}};

static const boost::container::flat_map<uint8_t, std::string>
    multiPchModeMisconfig = {
        {0x01, "BIOS did not set reset synchronization in multiPCH mode"},
        {0x02,
         "PMC indicates different non/legacy mode for the PCH than BMC set "
         "on the GPIO"},
        {0x03,
         "Misconfiguration MPCH support enabled due to BTG support enabled"}};

static const boost::container::flat_map<uint8_t, std::string>
    flashVerificationError = {
        {0x00, "OEM Public Key verification error"},
        {0x01, "Flash Descriptor Region Manifest verification error"},
        {0x02, "Soft Straps verification error"}};

namespace autoconfiguration
{

bool messageHook(const SELData& selData, std::string& eventId,
                 std::vector<std::string>& args)
{
    static const boost::container::flat_map<uint8_t, std::string> dcSource = {
        {0b00, "BMC"}, {0b01, "PSU"}, {0b10, "On-board power sensor"}};

    static const boost::container::flat_map<uint8_t, std::string>
        chassisSource = {{0b00, "BMC"},
                         {0b01, "PSU"},
                         {0b10, "On-board power sensor"},
                         {0b11, "Not supported"}};

    static const boost::container::flat_map<uint8_t, std::string>
        efficiencySource = {
            {0b00, "BMC"}, {0b01, "PSU"}, {0b11, "Not supported"}};

    static const boost::container::flat_map<uint8_t, std::string>
        unmanagedSource = {{0b00, "BMC"}, {0b01, "Estimated"}};

    static const boost::container::flat_map<uint8_t, std::string>
        failureReason = {{0b00, "BMC discovery failure"},
                         {0b01, "Insufficient factory configuration"},
                         {0b10, "Unknown sensor type"},
                         {0b11, "Other error encountered"}};

    auto succeeded = selData.eventData3 >> 7 & 0b1;
    if (succeeded)
    {
        eventId = "MEAutoConfigSuccess";

        auto dc = dcSource.find(selData.eventData3 >> 5 & 0b11);
        auto chassis = chassisSource.find(selData.eventData3 >> 3 & 0b11);
        auto efficiency = efficiencySource.find(selData.eventData3 >> 1 & 0b11);
        auto unmanaged = unmanagedSource.find(selData.eventData3 & 0b1);
        if (dc == dcSource.end() || chassis == chassisSource.end() ||
            efficiency == efficiencySource.end() ||
            unmanaged == unmanagedSource.end())
        {
            return false;
        }

        args.push_back(dc->second);
        args.push_back(chassis->second);
        args.push_back(efficiency->second);
        args.push_back(unmanaged->second);
    }
    else
    {
        eventId = "MEAutoConfigFailed";

        const auto it = failureReason.find(selData.eventData3 >> 5 & 0b11);
        if (it == failureReason.end())
        {
            return false;
        }

        args.push_back(it->second);
    }

    return true;
}
} // namespace autoconfiguration

namespace factory_reset
{
bool messageHook(const SELData& selData, std::string& eventId,
                 std::vector<std::string>& args)
{
    static const boost::container::flat_map<uint8_t, std::string>
        restoreToFactoryPreset = {
            {0x00,
             "Flash file system error detected. Automatic restore to factory "
             "presets has been triggered."},
            {0x01, "Automatic restore to factory presets has been completed."},
            {0x02, "Restore to factory presets triggered by Force ME Recovery "
                   "IPMI command has been completed."},
            {0x03,
             "Restore to factory presets triggered by AC power cycle with "
             "Recovery jumper asserted has been completed."}};

    auto param = restoreToFactoryPreset.find(selData.eventData3);
    if (param == restoreToFactoryPreset.end())
    {
        return false;
    }

    if (selData.eventData3 == 0x00)
    {
        eventId = "MEFactoryResetError";
    }
    else
    {
        eventId = "MEFactoryRestore";
    }

    args.push_back(param->second);
    return true;
}
} // namespace factory_reset

namespace flash_state
{
bool messageHook(const SELData& selData, std::string& eventId,
                 std::vector<std::string>& args)
{
    static const boost::container::flat_map<uint8_t, std::string>
        flashStateInformation = {
            {0x00,
             "Flash partition table, recovery image or factory presets image "
             "corrupted"},
            {0x01, "Flash erase limit has been reached"},
            {0x02,
             "Flash write limit has been reached. Writing to flash has been "
             "disabled"},
            {0x03, "Writing to the flash has been enabled"}};

    auto param = flashStateInformation.find(selData.eventData3);
    if (param == flashStateInformation.end())
    {
        return false;
    }

    if (selData.eventData3 == 0x03)
    {
        eventId = "MEFlashStateInformationWritingEnabled";
    }
    else
    {
        eventId = "MEFlashStateInformation";
    }

    args.push_back(param->second);
    return true;
}
} // namespace flash_state

static bool messageHook(const SELData& selData, std::string& eventId,
                        std::vector<std::string>& args)
{
    static const boost::container::flat_map<
        uint8_t,
        std::pair<std::string, std::optional<std::variant<utils::ParserFunc,
                                                          utils::MessageMap>>>>
        eventMap = {
            {0x00, {"MERecoveryGpioForced", {}}},
            {0x01, {"MEImageExecutionFailed", {}}},
            {0x02, {"MEFlashEraseError", {}}},
            {0x03, {{}, flash_state::messageHook}},
            {0x04, {"MEInternalError", {}}},
            {0x05, {"MEExceptionDuringShutdown", {}}},
            {0x06, {"MEDirectFlashUpdateRequested", {}}},
            {0x07, {"MEManufacturingError", manufacturingError}},
            {0x08, {{}, factory_reset::messageHook}},
            {0x09, {"MEFirmwareException", utils::logByteHex<2>}},
            {0x0A, {"MEFlashWearOutWarning", utils::logByteDec<2>}},
            {0x0D, {"MEPeciOverDmiError", peciOverDmiError}},
            {0x0E, {"MEMctpInterfaceError", mctpInterfaceError}},
            {0x0F, {{}, autoconfiguration::messageHook}},
            {0x10, {"MEUnsupportedFeature", unsupportedFeature}},
            {0x12, {"MECpuDebugCapabilityDisabled", {}}},
            {0x13, {"MEUmaError", umaError}},
            {0x16, {"MEPttHealthEvent", pttHealthEvent}},
            {0x17, {"MEBootGuardHealthEvent", bootGuardHealthEvent}},
            {0x18, {"MERestrictedMode", restrictedMode}},
            {0x19, {"MEMultiPchModeMisconfig", multiPchModeMisconfig}},
            {0x1A, {"MEFlashVerificationError", flashVerificationError}}};

    return utils::genericMessageHook(eventMap, selData, eventId, args);
}
} // namespace fw_status

static bool messageHook(const SELData& selData, std::string& eventId,
                        std::vector<std::string>& args)
{
    const HealthEventType healthEventType =
        static_cast<HealthEventType>(selData.offset);

    switch (healthEventType)
    {
        case HealthEventType::FirmwareStatus:
            return fw_status::messageHook(selData, eventId, args);
            break;

        case HealthEventType::SmbusLinkFailure:
            return smbus_failure::messageHook(selData, eventId, args);
            break;
    }

    return false;
}
} // namespace health_event

/**
 * @brief Main entry point for parsing ME IPMI Platform Events
 *
 * @brief selData - IPMI Platform Event structure
 * @brief ipmiRaw - the same event in raw binary form
 *
 * @returns true if event was successfully parsed and consumed
 */
bool messageHook(const SELData& selData, const std::string& ipmiRaw)
{
    const EventSensor meSensor = static_cast<EventSensor>(selData.sensorNum);
    std::string eventId;
    std::vector<std::string> args;

    switch (meSensor)
    {
        case EventSensor::MeFirmwareHealth:
            if (health_event::messageHook(selData, eventId, args))
            {
                utils::storeRedfishEvent(ipmiRaw, eventId, args);
                return true;
            }
            break;
    }

    return defaultMessageHook(ipmiRaw);
}
} // namespace intel_oem::ipmi::sel::redfish_hooks::me