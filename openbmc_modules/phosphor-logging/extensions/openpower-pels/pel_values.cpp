/**
 * Copyright © 2019 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pel_values.hpp"

#include <algorithm>
#include <cassert>

namespace openpower
{
namespace pels
{
namespace pel_values
{

/**
 * The possible values for the subsystem field  in the User Header.
 */
const PELValues subsystemValues = {
    {0x10, "processor", "Processor Subsystem"},
    {0x11, "processor_fru", "Processor FRU"},
    {0x12, "processor_chip", "Processor Chip Cache"},
    {0x13, "processor_unit", "Processor Unit (CPU)"},
    {0x14, "processor_bus", "Processor Bus Controller"},

    {0x20, "memory", "Memory Subsystem"},
    {0x21, "memory_ctlr", "Memory Controller"},
    {0x22, "memory_bus", "Memory Bus Interface"},
    {0x23, "memory_dimm", "Memory DIMM"},
    {0x24, "memory_fru", "Memory Card/FRU"},
    {0x25, "external_cache", "External Cache"},

    {0x30, "io", "I/O Subsystem"},
    {0x31, "io_hub", "I/O Hub"},
    {0x32, "io_bridge", "I/O Bridge"},
    {0x33, "io_bus", "I/O bus interface"},
    {0x34, "io_processor", "I/O Processor"},
    {0x35, "io_hub_other", "SMA Hub"},
    {0x38, "phb", "PCI Bridge Chip"},

    {0x40, "io_adapter", "I/O Adapter Subsystem"},
    {0x41, "io_adapter_comm", "I/O Adapter Communication"},
    {0x46, "io_device", "I/O Device Subsystem"},
    {0x47, "io_device_dasd", "I/O Device Disk"},
    {0x4C, "io_external_general", "I/O External Peripheral"},
    {0x4D, "io_external_workstation",
     "I/O External Peripheral Local Work Station"},
    {0x4E, "io_storage_mezz", "I/O Storage Mezza Expansion Subsystem"},

    {0x50, "cec_hardware", "CEC Hardware Subsystem"},
    {0x51, "cec_sp_a", "CEC Hardware: Service Processor A"},
    {0x52, "cec_sp_b", "CEC Hardware: Service Processor B"},
    {0x53, "cec_node_controller", "CEC Hardware: Node Controller"},
    {0x55, "cec_vpd", "CEC Hardware: VPD Interface"},
    {0x56, "cec_i2c", "CEC Hardware: I2C Devices"},
    {0x57, "cec_chip_iface", "CEC Hardware: CEC Chip Interface"},
    {0x58, "cec_clocks", "CEC Hardware: Clock"},
    {0x59, "cec_op_panel", "CEC Hardware: Operator Panel"},
    {0x5A, "cec_tod", "CEC Hardware: Time-Of-Day Hardware"},
    {0x5B, "cec_storage_device", "CEC Hardware: Memory Device"},
    {0x5C, "cec_sp_hyp_iface",
     "CEC Hardware: Hypervisor-Service Processor Interface"},
    {0x5D, "cec_service_network", "CEC Hardware: Service Network"},
    {0x5E, "cec_sp_hostboot_iface",
     "CEC Hardware: Hostboot-Service Processor Interface"},

    {0x60, "power", "Power/Cooling Subsystem"},
    {0x61, "power_supply", "Power Supply"},
    {0x62, "power_control_hw", "Power Control Hardware"},
    {0x63, "power_fans", "Fan (AMD)"},
    {0x64, "power_sequencer", "Digital Power Supply Subsystem"},

    {0x70, "others", "Miscellaneous"},
    {0x71, "other_hmc", "HMC Subsystem & Hardware"},
    {0x72, "other_test_tool", "Test Tool"},
    {0x73, "other_media", "Removable Media"},
    {0x74, "other_multiple_subsystems", "Multiple Subsystems"},
    {0x75, "other_na", "Not Applicable"},
    {0x76, "other_info_src", "Miscellaneous"},

    {0x7A, "surv_hyp_lost_sp",
     "Hypervisor lost communication with service processor"},
    {0x7B, "surv_sp_lost_hyp",
     "Service processor lost communication with Hypervisor"},
    {0x7C, "surv_sp_lost_hmc", "Service processor lost communication with HMC"},
    {0x7D, "surv_hmc_lost_lpar",
     "HMC lost communication with logical partition"},
    {0x7E, "surv_hmc_lost_bpa", "HMC lost communication with BPA"},
    {0x7F, "surv_hmc_lost_hmc", "HMC lost communication with another HMC"},

    {0x80, "platform_firmware", "Platform Firmware"},
    {0x81, "sp_firmware", "Service Processor Firmware"},
    {0x82, "hyp_firmware", "System Hypervisor Firmware"},
    {0x83, "partition_firmware", "Partition Firmware"},
    {0x84, "slic_firmware", "SLIC Firmware"},
    {0x85, "spcn_firmware", "System Power Control Network Firmware"},
    {0x86, "bulk_power_firmware_side_a", "Bulk Power Firmware Side A"},
    {0x87, "hmc_code_firmware", "HMC Code"},
    {0x88, "bulk_power_firmware_side_b", "Bulk Power Firmware Side B"},
    {0x89, "virtual_sp", "Virtual Service Processor Firmware"},
    {0x8A, "hostboot", "HostBoot"},
    {0x8B, "occ", "OCC"},
    {0x8D, "bmc_firmware", "BMC Firmware"},

    {0x90, "software", "Software"},
    {0x91, "os_software", "Operating System software"},
    {0x92, "xpf_software", "XPF software"},
    {0x93, "app_software", "Application software"},

    {0xA0, "ext_env", "External Environment"},
    {0xA1, "input_power_source", "Input Power Source (ac)"},
    {0xA2, "ambient_temp", "Room Ambient Temperature"},
    {0xA3, "user_error", "User Error"},
    {0xA4, "corrosion", "Corrosion"}};

/**
 * The possible values for the severity field in the User Header.
 */
const PELValues severityValues = {
    {0x00, "non_error", "Informational Event"},

    {0x10, "recovered", "Recovered Error"},
    {0x20, "predictive", "Predictive Error"},
    {0x21, "predictive_degraded_perf",
     "Predictive Error, Degraded Performance"},
    {0x22, "predictive_reboot", "Predictive Error, Correctable"},
    {0x23, "predictive_reboot_degraded",
     "Predictive Error, Correctable, Degraded"},
    {0x24, "predictive_redundancy_loss", "Predictive Error, Redundancy Lost"},

    {0x40, "unrecoverable", "Unrecoverable Error"},
    {0x41, "unrecoverable_degraded_perf",
     "Unrecoverable Error, Degraded Performance"},
    {0x44, "unrecoverable_redundancy_loss",
     "Unrecoverable Error, Loss of Redundancy"},
    {0x45, "unrecoverable_redundancy_loss_perf",
     "Unrecoverable, Loss of Redundancy + Performance"},
    {0x48, "unrecoverable_loss_of_function",
     "Unrecoverable Error, Loss of Function"},

    {0x50, "critical", "Critical Error, Scope of Failure unknown"},
    {0x51, "critical_system_term", "Critical Error, System Termination"},
    {0x52, "critical_imminent_failure",
     "Critical Error, System Failure likely or imminent"},
    {0x53, "critical_partition_term",
     "Critical Error, Partition(s) Termination"},
    {0x54, "critical_partition_imminent_failure",
     "Critical Error, Partition(s) Failure likely or imminent"},

    {0x60, "diagnostic_error", "Error detected during diagnostic test"},
    {0x61, "diagnostic_error_incorrect_results",
     "Diagostic error, resource w/incorrect results"},

    {0x71, "symptom_recovered", "Symptom Recovered"},
    {0x72, "symptom_predictive", "Symptom Predictive"},
    {0x74, "symptom_unrecoverable", "Symptom Unrecoverable"},
    {0x75, "symptom_critical", "Symptom Critical"},
    {0x76, "symptom_diag_err", "Symptom Diag Err"}};

/**
 * The possible values for the Event Type field in the User Header.
 */
const PELValues eventTypeValues = {
    {0x00, "na", "Not Applicable"},
    {0x01, "misc_information_only", "Miscellaneous, Informational Only"},
    {0x02, "tracing_event", "Tracing Event"},
    {0x08, "dump_notification", "Dump Notification"}};

/**
 * The possible values for the Event Scope field in the User Header.
 */
const PELValues eventScopeValues = {
    {0x01, "single_partition", "Single Partition"},
    {0x02, "multiple_partitions", "Multiple Partitions"},
    {0x03, "entire_platform", "Entire Platform"},
    {0x04, "possibly_multiple_platforms", "Multiple Platforms"}};

/**
 * The possible values for the Action Flags field in the User Header.
 */
const PELValues actionFlagsValues = {
    {0x8000, "service_action", "Service Action Required"},
    {0x4000, "hidden", "Event not customer viewable"},
    {0x2000, "report", "Report Externally"},
    {0x1000, "dont_report", "Do Not Report To Hypervisor"},
    {0x0800, "call_home", "HMC Call Home"},
    {0x0400, "isolation_incomplete",
     "Isolation Incomplete, further analysis required"},
    {0x0100, "termination", "Service Processor Call Home Required"}};

/**
 * The possible values for the Callout Priority field in the SRC.
 */
const PELValues calloutPriorityValues = {
    {0x48, "high", "Mandatory, replace all with this type as a unit"},
    {0x4D, "medium", "Medium Priority"},
    {0x41, "medium_group_a", "Medium Priority A, replace these as a group"},
    {0x42, "medium_group_b", "Medium Priority B, replace these as a group"},
    {0x43, "medium_group_c", "Medium Priority C, replace these as a group"},
    {0x4C, "low", "Lowest priority replacement"}};

/**
 * @brief Map of the registry names for the maintenance procedures
 *        to their actual names.
 */
const std::map<std::string, std::string> maintenanceProcedures = {
    {"no_vpd_for_fru", "BMCSP01"}, {"bmc_code", "BMCSP02"}};

/**
 * @brief Map of the registry names for the symbolic FRUs to their
 *        actual names.
 */
const std::map<std::string, std::string> symbolicFRUs = {
    {"service_docs", "SVCDOCS"}};

PELValues::const_iterator findByValue(uint32_t value, const PELValues& fields)
{
    return std::find_if(fields.begin(), fields.end(),
                        [value](const auto& entry) {
                            return value == std::get<fieldValuePos>(entry);
                        });
}

PELValues::const_iterator findByName(const std::string& name,
                                     const PELValues& fields)

{
    return std::find_if(fields.begin(), fields.end(),
                        [&name](const auto& entry) {
                            return name == std::get<registryNamePos>(entry);
                        });
}

/**
 * @brief Map for section IDs
 */
const std::map<std::string, std::string> sectionTitles = {
    {"PH", "Private Header"},
    {"UH", "User Header"},
    {"PS", "Primary SRC"},
    {"SS", "Secondary SRC"},
    {"EH", "Extended User Header"},
    {"MT", "Failing MTMS"},
    {"DH", "Dump Location"},
    {"SW", "Firmware Error"},
    {"LP", "Impacted Partition"},
    {"LR", "Logical Resource"},
    {"HM", "HMC ID"},
    {"EP", "EPOW"},
    {"IE", "IO Event"},
    {"MI", "MFG Info"},
    {"CH", "Call Home"},
    {"UD", "User Data"},
    {"EI", "Env Info"},
    {"ED", "Extended User Data"}};

/**
 * @brief Map for Procedure Descriptions
 */
const std::map<std::string, std::string> procedureDesc = {{"TODO", "TODO"}};

/**
 * @brief Map for Callout Failing Component Types
 */
const std::map<uint8_t, std::string> failingComponentType = {
    {0x10, "Normal Hardware FRU"},
    {0x20, "Code FRU"},
    {0x30, "Configuration error, configuration procedure required"},
    {0x40, "Maintenance Procedure Required"},
    {0x90, "External FRU"},
    {0xA0, "External Code FRU"},
    {0xB0, "Tool FRU"},
    {0xC0, "Symbolic FRU"},
    {0xE0, "Symbolic FRU with trusted location code"}};

/**
 * @brief Map for Boolean value
 */
const std::map<bool, std::string> boolString = {{true, "True"},
                                                {false, "False"}};

/**
 * @brief Map for creator IDs
 */
const std::map<std::string, std::string> creatorIDs = {

    {"O", "BMC"},      {"C", "HMC"},      {"H", "PHYP"}, {"L", "Partition FW"},
    {"S", "SLIC"},     {"B", "Hostboot"}, {"T", "OCC"},  {"M", "I/O Drawer"},
    {"K", "Sapphire"}, {"P", "PowerNV"}};

/**
 * @brief Map for transmission states
 */
const std::map<TransmissionState, std::string> transmissionStates = {
    {TransmissionState::newPEL, "Not Sent"},
    {TransmissionState::badPEL, "Rejected"},
    {TransmissionState::sent, "Sent"},
    {TransmissionState::acked, "Acked"}};

std::string getValue(const uint8_t field, const pel_values::PELValues& values)
{

    auto tmp = pel_values::findByValue(field, values);
    if (tmp != values.end())
    {
        return std::get<pel_values::descriptionPos>(*tmp);
    }
    else
    {
        return "invalid";
    }
}

std::vector<std::string> getValuesBitwise(uint16_t value,
                                          const pel_values::PELValues& table)
{
    std::vector<std::string> foundValues;
    std::for_each(
        table.begin(), table.end(), [&value, &foundValues](const auto& entry) {
            if (value & std::get<fieldValuePos>(entry))
            {
                foundValues.push_back(std::get<descriptionPos>(entry));
            }
        });
    return foundValues;
}

} // namespace pel_values
} // namespace pels
} // namespace openpower
