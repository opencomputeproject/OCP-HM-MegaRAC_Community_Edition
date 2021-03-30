#pragma once

#include "config.h"

#include "activation.hpp"
#include "association_interface.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "version.hpp"

#include <filesystem>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Association/Definitions/server.hpp>
#include <xyz/openbmc_project/Collection/DeleteAll/server.hpp>

class TestItemUpdater;

namespace phosphor
{
namespace software
{
namespace updater
{

class Version;

using ItemUpdaterInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Association::server::Definitions>;

namespace MatchRules = sdbusplus::bus::match::rules;

namespace fs = std::filesystem;

/** @class ItemUpdater
 *  @brief Manages the activation of the PSU version items.
 */
class ItemUpdater : public ItemUpdaterInherit,
                    public AssociationInterface,
                    public ActivationListener
{
    friend class ::TestItemUpdater;

  public:
    /** @brief Constructs ItemUpdater
     *
     * @param[in] bus    - The D-Bus bus object
     * @param[in] path   - The D-Bus path
     */
    ItemUpdater(sdbusplus::bus::bus& bus, const std::string& path) :
        ItemUpdaterInherit(bus, path.c_str()), bus(bus),
        versionMatch(bus,
                     MatchRules::interfacesAdded() +
                         MatchRules::path(SOFTWARE_OBJPATH),
                     std::bind(std::mem_fn(&ItemUpdater::createActivation),
                               this, std::placeholders::_1))
    {
        processPSUImage();
        processStoredImage();
        syncToLatestImage();
    }

    /** @brief Deletes version
     *
     *  @param[in] versionId - Id of the version to delete
     */
    void erase(const std::string& versionId);

    /** @brief Creates an active association to the
     *  newly active software image
     *
     * @param[in]  path - The path to create the association to.
     */
    void createActiveAssociation(const std::string& path) override;

    /** @brief Add the functional association to the
     *  new "running" PSU images
     *
     * @param[in]  path - The path to add the association to.
     */
    void addFunctionalAssociation(const std::string& path) override;

    /** @brief Add the updateable association to the
     *  "running" PSU software image
     *
     * @param[in]  path - The path to create the association.
     */
    void addUpdateableAssociation(const std::string& path) override;

    /** @brief Removes the associations from the provided software image path
     *
     * @param[in]  path - The path to remove the association from.
     */
    void removeAssociation(const std::string& path) override;

    /** @brief Notify a PSU is updated
     *
     * @param[in]  versionId - The versionId of the activation
     * @param[in]  psuInventoryPath - The PSU inventory path that is updated
     */
    void onUpdateDone(const std::string& versionId,
                      const std::string& psuInventoryPath) override;

  private:
    /** @brief Callback function for Software.Version match.
     *  @details Creates an Activation D-Bus object.
     *
     * @param[in]  msg       - Data associated with subscribed signal
     */
    void createActivation(sdbusplus::message::message& msg);

    using Properties =
        std::map<std::string, utils::UtilsInterface::PropertyType>;

    /** @brief Callback function for PSU inventory match.
     *  @details Update an Activation D-Bus object for PSU inventory.
     *
     * @param[in]  msg       - Data associated with subscribed signal
     */
    void onPsuInventoryChangedMsg(sdbusplus::message::message& msg);

    /** @brief Callback function for PSU inventory match.
     *  @details Update an Activation D-Bus object for PSU inventory.
     *
     * @param[in]  psuPath - The PSU inventory path
     * @param[in]  properties - The updated properties
     */
    void onPsuInventoryChanged(const std::string& psuPath,
                               const Properties& properties);

    /** @brief Create Activation object */
    std::unique_ptr<Activation> createActivationObject(
        const std::string& path, const std::string& versionId,
        const std::string& extVersion, Activation::Status activationStatus,
        const AssociationList& assocs, const std::string& filePath);

    /** @brief Create Version object */
    std::unique_ptr<Version>
        createVersionObject(const std::string& objPath,
                            const std::string& versionId,
                            const std::string& versionString,
                            sdbusplus::xyz::openbmc_project::Software::server::
                                Version::VersionPurpose versionPurpose);

    /** @brief Create Activation and Version object for PSU inventory
     *  @details If the same version exists for multiple PSUs, just add
     *           related association, instead of creating new objects.
     * */
    void createPsuObject(const std::string& psuInventoryPath,
                         const std::string& psuVersion);

    /** @brief Remove Activation and Version object for PSU inventory
     *  @details If the same version exists for mutliple PSUs, just remove
     *           related association.
     *           If the version has no association, the Activation and
     *           Version object will be removed
     */
    void removePsuObject(const std::string& psuInventoryPath);

    /**
     * @brief Create and populate the active PSU Version.
     */
    void processPSUImage();

    /** @brief Create PSU Version from stored images */
    void processStoredImage();

    /** @brief Scan a directory and create PSU Version from stored images */
    void scanDirectory(const fs::path& p);

    /** @brief Get the versionId of the latest PSU version */
    std::optional<std::string> getLatestVersionId();

    /** @brief Update PSUs to the latest version */
    void syncToLatestImage();

    /** @brief Invoke the activation via DBus */
    void invokeActivation(const std::unique_ptr<Activation>& activation);

    /** @brief Persistent sdbusplus D-Bus bus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief Persistent map of Activation D-Bus objects and their
     * version id */
    std::map<std::string, std::unique_ptr<Activation>> activations;

    /** @brief Persistent map of Version D-Bus objects and their
     * version id */
    std::map<std::string, std::unique_ptr<Version>> versions;

    /** @brief The reference map of PSU Inventory objects and the
     * Activation*/
    std::map<std::string, const std::unique_ptr<Activation>&>
        psuPathActivationMap;

    /** @brief sdbusplus signal match for PSU Software*/
    sdbusplus::bus::match_t versionMatch;

    /** @brief sdbusplus signal matches for PSU Inventory */
    std::vector<sdbusplus::bus::match_t> psuMatches;

    /** @brief This entry's associations */
    AssociationList assocs;

    /** @brief A collection of the version strings */
    std::set<std::string> versionStrings;

    /** @brief A struct to hold the PSU present status and model */
    struct psuStatus
    {
        bool present;
        std::string model;
    };

    /** @brief The map of PSU inventory path and the psuStatus
     *
     * It is used to handle psu inventory changed event, that only create psu
     * software object when a PSU is present and the model is retrieved */
    std::map<std::string, psuStatus> psuStatusMap;
};

} // namespace updater
} // namespace software
} // namespace phosphor
