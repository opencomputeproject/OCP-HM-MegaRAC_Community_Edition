#pragma once

#include "pel_types.hpp"

#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace openpower
{
namespace pels
{
namespace pel_values
{

// The actual value as it shows up in the PEL
const int fieldValuePos = 0;

// The name of the value as specified in the message registry
const int registryNamePos = 1;

// The description of the field, used by PEL parsers
const int descriptionPos = 2;

using PELFieldValue = std::tuple<uint32_t, const char*, const char*>;
using PELValues = std::vector<PELFieldValue>;

const std::string sectionVer = "Section Version";
const std::string subSection = "Sub-section type";
const std::string createdBy = "Created by";

/**
 * @brief Helper function to get values from lookup tables.
 * @return std::string - the value
 * @param[in] uint8_t - field to get value for
 * @param[in] PELValues - lookup table
 */
std::string getValue(const uint8_t field, const pel_values::PELValues& values);

/**
 * @brief Helper function to get value vector from lookup tables.
 *
 * @param[in] value - the value to lookup
 * @param[in] table - lookup table
 *
 * @return std::vector<std::string> - the value vector
 */
std::vector<std::string> getValuesBitwise(uint16_t value,
                                          const pel_values::PELValues& table);
/**
 * @brief Find the desired entry in a PELValues table based on the
 *        field value.
 *
 * @param[in] value - the PEL value to find
 * @param[in] fields - the PEL values table to use
 *
 * @return PELValues::const_iterator - an iterator to the table entry
 */
PELValues::const_iterator findByValue(uint32_t value, const PELValues& fields);

/**
 * @brief Find the desired entry in a PELValues table based on the
 *        field message registry name.
 *
 * @param[in] name - the PEL message registry enum name
 * @param[in] fields - the PEL values table to use
 *
 * @return PELValues::const_iterator - an iterator to the table entry
 */
PELValues::const_iterator findByName(const std::string& name,
                                     const PELValues& fields);

/**
 * @brief The values for the 'subsystem' field in the User Header
 */
extern const PELValues subsystemValues;

/**
 * @brief The values for the 'severity' field in the User Header
 */
extern const PELValues severityValues;

/**
 * @brief The values for the 'Event Type' field in the User Header
 */
extern const PELValues eventTypeValues;

/**
 * @brief The values for the 'Event Scope' field in the User Header
 */
extern const PELValues eventScopeValues;

/**
 * @brief The values for the 'Action Flags' field in the User Header
 */
extern const PELValues actionFlagsValues;

/**
 * @brief The values for callout priorities in the SRC section
 */
extern const PELValues calloutPriorityValues;

/**
 * @brief Map for section IDs
 */
extern const std::map<std::string, std::string> sectionTitles;

/**
 * @brief Map for creator IDs
 */
extern const std::map<std::string, std::string> creatorIDs;

/**
 * @brief Map for transmission states
 */
extern const std::map<TransmissionState, std::string> transmissionStates;

/**
 * @brief Map for Procedure Descriptions
 */
extern const std::map<std::string, std::string> procedureDesc;

/**
 * @brief Map for Callout Failing Component Types
 */
extern const std::map<uint8_t, std::string> failingComponentType;

/**
 * @brief Map for Boolean value
 */
extern const std::map<bool, std::string> boolString;

/**
 * @brief Map for maintenance procedures
 */
extern const std::map<std::string, std::string> maintenanceProcedures;

/**
 * @brief Map for symbolic FRUs.
 */
extern const std::map<std::string, std::string> symbolicFRUs;

} // namespace pel_values
} // namespace pels
} // namespace openpower
