#pragma once

#include "activation.hpp"
#include "item_updater_helper.hpp"
#include "version.hpp"
#include "xyz/openbmc_project/Collection/DeleteAll/server.hpp"

#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Association/Definitions/server.hpp>
#include <xyz/openbmc_project/Common/FactoryReset/server.hpp>
#include <xyz/openbmc_project/Control/FieldMode/server.hpp>

#include <string>

namespace phosphor
{
namespace software
{
namespace updater
{

using ItemUpdaterInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Common::server::FactoryReset,
    sdbusplus::xyz::openbmc_project::Control::server::FieldMode,
    sdbusplus::xyz::openbmc_project::Association::server::Definitions,
    sdbusplus::xyz::openbmc_project::Collection::server::DeleteAll>;

namespace MatchRules = sdbusplus::bus::match::rules;
using VersionClass = phosphor::software::manager::Version;
using AssociationList =
    std::vector<std::tuple<std::string, std::string, std::string>>;

/** @class ItemUpdater
 *  @brief Manages the activation of the BMC version items.
 */
class ItemUpdater : public ItemUpdaterInherit
{
  public:
    /*
     * @brief Types of Activation status for image validation.
     */
    enum class ActivationStatus
    {
        ready,
        invalid,
        active
    };

    /** @brief Constructs ItemUpdater
     *
     * @param[in] bus    - The D-Bus bus object
     */
    ItemUpdater(sdbusplus::bus::bus& bus, const std::string& path) :
        ItemUpdaterInherit(bus, path.c_str(), false), bus(bus), helper(bus),
        versionMatch(bus,
                     MatchRules::interfacesAdded() +
                         MatchRules::path("/xyz/openbmc_project/software"),
                     std::bind(std::mem_fn(&ItemUpdater::createActivation),
                               this, std::placeholders::_1))
    {
        setBMCInventoryPath();
        processBMCImage();
        restoreFieldModeStatus();
        emit_object_added();
    };

    /** @brief Save priority value to persistent storage (flash and optionally
     *  a U-Boot environment variable)
     *
     *  @param[in] versionId - The Id of the version
     *  @param[in] value - The priority value
     *  @return None
     */
    void savePriority(const std::string& versionId, uint8_t value);

    /** @brief Sets the given priority free by incrementing
     *  any existing priority with the same value by 1
     *
     *  @param[in] value - The priority that needs to be set free.
     *  @param[in] versionId - The Id of the version for which we
     *                         are trying to free up the priority.
     *  @return None
     */
    void freePriority(uint8_t value, const std::string& versionId);

    /**
     * @brief Create and populate the active BMC Version.
     */
    void processBMCImage();

    /**
     * @brief Erase specified entry D-Bus object
     *        if Action property is not set to Active
     *
     * @param[in] entryId - unique identifier of the entry
     */
    void erase(std::string entryId);

    /**
     * @brief Deletes all versions except for the current one
     */
    void deleteAll();

    /** @brief Creates an active association to the
     *  newly active software image
     *
     * @param[in]  path - The path to create the association to.
     */
    void createActiveAssociation(const std::string& path);

    /** @brief Removes the associations from the provided software image path
     *
     * @param[in]  path - The path to remove the associations from.
     */
    void removeAssociations(const std::string& path);

    /** @brief Determine if the given priority is the lowest
     *
     *  @param[in] value - The priority that needs to be checked.
     *
     *  @return boolean corresponding to whether the given
     *      priority is lowest.
     */
    bool isLowestPriority(uint8_t value);

    /**
     * @brief Updates the U-Boot variables to point to the requested
     *        versionId, so that the systems boots from this version on
     *        the next reboot.
     *
     * @param[in] versionId - The version to point the system to boot from.
     */
    void updateUbootEnvVars(const std::string& versionId);

    /**
     * @brief Updates the uboot variables to point to BMC version with lowest
     *        priority, so that the system boots from this version on the
     *        next boot.
     */
    void resetUbootEnvVars();

    /** @brief Brings the total number of active BMC versions to
     *         ACTIVE_BMC_MAX_ALLOWED -1. This function is intended to be
     *         run before activating a new BMC version. If this function
     *         needs to delete any BMC version(s) it will delete the
     *         version(s) with the highest priority, skipping the
     *         functional BMC version.
     *
     * @param[in] caller - The Activation object that called this function.
     */
    void freeSpace(Activation& caller);

    /** @brief Creates a updateable association to the
     *  "running" BMC software image
     *
     * @param[in]  path - The path to create the association.
     */
    void createUpdateableAssociation(const std::string& path);

    /** @brief Persistent map of Version D-Bus objects and their
     * version id */
    std::map<std::string, std::unique_ptr<VersionClass>> versions;

  private:
    /** @brief Callback function for Software.Version match.
     *  @details Creates an Activation D-Bus object.
     *
     * @param[in]  msg       - Data associated with subscribed signal
     */
    void createActivation(sdbusplus::message::message& msg);

    /**
     * @brief Validates the presence of SquashFS image in the image dir.
     *
     * @param[in]  filePath  - The path to the image dir.
     * @param[out] result    - ActivationStatus Enum.
     *                         ready if validation was successful.
     *                         invalid if validation fail.
     *                         active if image is the current version.
     *
     */
    ActivationStatus validateSquashFSImage(const std::string& filePath);

    /** @brief BMC factory reset - marks the read-write partition for
     * recreation upon reboot. */
    void reset() override;

    /**
     * @brief Enables field mode, if value=true.
     *
     * @param[in]  value  - If true, enables field mode.
     * @param[out] result - Returns the current state of field mode.
     *
     */
    bool fieldModeEnabled(bool value) override;

    /** @brief Sets the BMC inventory item path under
     *  /xyz/openbmc_project/inventory/system/chassis/. */
    void setBMCInventoryPath();

    /** @brief The path to the BMC inventory item. */
    std::string bmcInventoryPath;

    /** @brief Restores field mode status on reboot. */
    void restoreFieldModeStatus();

    /** @brief Creates a functional association to the
     *  "running" BMC software image
     *
     * @param[in]  path - The path to create the association to.
     */
    void createFunctionalAssociation(const std::string& path);

    /** @brief Persistent sdbusplus D-Bus bus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief The helper of image updater. */
    Helper helper;

    /** @brief Persistent map of Activation D-Bus objects and their
     * version id */
    std::map<std::string, std::unique_ptr<Activation>> activations;

    /** @brief sdbusplus signal match for Software.Version */
    sdbusplus::bus::match_t versionMatch;

    /** @brief This entry's associations */
    AssociationList assocs = {};

    /** @brief Clears read only partition for
     * given Activation D-Bus object.
     *
     * @param[in]  versionId - The version id.
     */
    void removeReadOnlyPartition(std::string versionId);

    /** @brief Copies U-Boot from the currently booted BMC chip to the
     *  alternate chip.
     */
    void mirrorUbootToAlt();
};

} // namespace updater
} // namespace software
} // namespace phosphor
