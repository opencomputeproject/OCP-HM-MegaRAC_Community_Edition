#pragma once

#include "config.h"

#include "version.hpp"

#include <string>

namespace phosphor
{
namespace software
{
namespace updater
{

using VersionPurpose =
    sdbusplus::xyz::openbmc_project::Software::server::Version::VersionPurpose;

/** @brief Serialization function - stores priority information to file
 *  @param[in] versionId - The version for which to store information.
 *  @param[in] priority - RedundancyPriority value for that version.
 **/
void storePriority(const std::string& versionId, uint8_t priority);

/** @brief Serialization function - stores purpose information to file
 *  @param[in] versionId - The version for which to store information.
 *  @param[in] purpose - VersionPurpose value for that version.
 **/
void storePurpose(const std::string& versionId, VersionPurpose purpose);

/** @brief Serialization function - restores priority information from file
 *  @param[in] versionId - The version for which to retrieve information.
 *  @param[in] priority - RedundancyPriority reference for that version.
 *  @return true if restore was successful, false if not
 **/
bool restorePriority(const std::string& versionId, uint8_t& priority);

/** @brief Serialization function - restores purpose information from file
 *  @param[in] versionId - The version for which to retrieve information.
 *  @param[in] purpose - VersionPurpose reference for that version.
 *  @return true if restore was successful, false if not
 **/
bool restorePurpose(const std::string& versionId, VersionPurpose& purpose);

/** @brief Removes the serial directory for a given version.
 *  @param[in] versionId - The version for which to remove a file, if it exists.
 **/
void removePersistDataDirectory(const std::string& versionId);

} // namespace updater
} // namespace software
} // namespace phosphor
