#include "config.h"

#include "item_updater.hpp"

#include "images.hpp"
#include "serialize.hpp"
#include "version.hpp"
#include "xyz/openbmc_project/Software/Version/server.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Software/Image/error.hpp>

#include <filesystem>
#include <fstream>
#include <queue>
#include <set>
#include <string>
#include <thread>

namespace phosphor
{
namespace software
{
namespace updater
{

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::Software::server;
namespace control = sdbusplus::xyz::openbmc_project::Control::server;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Software::Image::Error;
using namespace phosphor::software::image;
namespace fs = std::filesystem;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;

void ItemUpdater::createActivation(sdbusplus::message::message& msg)
{

    using SVersion = server::Version;
    using VersionPurpose = SVersion::VersionPurpose;
    using VersionClass = phosphor::software::manager::Version;

    sdbusplus::message::object_path objPath;
    auto purpose = VersionPurpose::Unknown;
    std::string version;
    std::map<std::string, std::map<std::string, std::variant<std::string>>>
        interfaces;
    msg.read(objPath, interfaces);
    std::string path(std::move(objPath));
    std::string filePath;

    for (const auto& intf : interfaces)
    {
        if (intf.first == VERSION_IFACE)
        {
            for (const auto& property : intf.second)
            {
                if (property.first == "Purpose")
                {
                    auto value = SVersion::convertVersionPurposeFromString(
                        std::get<std::string>(property.second));
                    if (value == VersionPurpose::BMC ||
#ifdef HOST_BIOS_UPGRADE
                        value == VersionPurpose::Host ||
#endif
                        value == VersionPurpose::System)
                    {
                        purpose = value;
                    }
                }
                else if (property.first == "Version")
                {
                    version = std::get<std::string>(property.second);
                }
            }
        }
        else if (intf.first == FILEPATH_IFACE)
        {
            for (const auto& property : intf.second)
            {
                if (property.first == "Path")
                {
                    filePath = std::get<std::string>(property.second);
                }
            }
        }
    }
    if (version.empty() || filePath.empty() ||
        purpose == VersionPurpose::Unknown)
    {
        return;
    }

    // Version id is the last item in the path
    auto pos = path.rfind("/");
    if (pos == std::string::npos)
    {
        log<level::ERR>("No version id found in object path",
                        entry("OBJPATH=%s", path.c_str()));
        return;
    }

    auto versionId = path.substr(pos + 1);

    if (activations.find(versionId) == activations.end())
    {
        // Determine the Activation state by processing the given image dir.
        auto activationState = server::Activation::Activations::Invalid;
        ItemUpdater::ActivationStatus result;
        if (purpose == VersionPurpose::BMC || purpose == VersionPurpose::System)
            result = ItemUpdater::validateSquashFSImage(filePath);
        else
            result = ItemUpdater::ActivationStatus::ready;

        AssociationList associations = {};

        if (result == ItemUpdater::ActivationStatus::ready)
        {
            activationState = server::Activation::Activations::Ready;
            // Create an association to the BMC inventory item
            associations.emplace_back(
                std::make_tuple(ACTIVATION_FWD_ASSOCIATION,
                                ACTIVATION_REV_ASSOCIATION, bmcInventoryPath));
        }

        activations.insert(std::make_pair(
            versionId,
            std::make_unique<Activation>(bus, path, *this, versionId,
                                         activationState, associations)));

        auto versionPtr = std::make_unique<VersionClass>(
            bus, path, version, purpose, filePath,
            std::bind(&ItemUpdater::erase, this, std::placeholders::_1));
        versionPtr->deleteObject =
            std::make_unique<phosphor::software::manager::Delete>(bus, path,
                                                                  *versionPtr);
        versions.insert(std::make_pair(versionId, std::move(versionPtr)));
    }
    return;
}

void ItemUpdater::processBMCImage()
{
    using VersionClass = phosphor::software::manager::Version;

    // Check MEDIA_DIR and create if it does not exist
    try
    {
        if (!fs::is_directory(MEDIA_DIR))
        {
            fs::create_directory(MEDIA_DIR);
        }
    }
    catch (const fs::filesystem_error& e)
    {
        log<level::ERR>("Failed to prepare dir", entry("ERR=%s", e.what()));
        return;
    }

    // Read os-release from /etc/ to get the functional BMC version
    auto functionalVersion = VersionClass::getBMCVersion(OS_RELEASE_FILE);

    // Read os-release from folders under /media/ to get
    // BMC Software Versions.
    for (const auto& iter : fs::directory_iterator(MEDIA_DIR))
    {
        auto activationState = server::Activation::Activations::Active;
        static const auto BMC_RO_PREFIX_LEN = strlen(BMC_ROFS_PREFIX);

        // Check if the BMC_RO_PREFIXis the prefix of the iter.path
        if (0 ==
            iter.path().native().compare(0, BMC_RO_PREFIX_LEN, BMC_ROFS_PREFIX))
        {
            // Get the version to calculate the id
            fs::path releaseFile(OS_RELEASE_FILE);
            auto osRelease = iter.path() / releaseFile.relative_path();
            if (!fs::is_regular_file(osRelease))
            {
                log<level::ERR>(
                    "Failed to read osRelease",
                    entry("FILENAME=%s", osRelease.string().c_str()));

                // Try to get the version id from the mount directory name and
                // call to delete it as this version may be corrupted. Dynamic
                // volumes created by the UBI layout for example have the id in
                // the mount directory name. The worst that can happen is that
                // erase() is called with an non-existent id and returns.
                auto id = iter.path().native().substr(BMC_RO_PREFIX_LEN);
                ItemUpdater::erase(id);

                continue;
            }
            auto version = VersionClass::getBMCVersion(osRelease);
            if (version.empty())
            {
                log<level::ERR>(
                    "Failed to read version from osRelease",
                    entry("FILENAME=%s", osRelease.string().c_str()));

                // Try to delete the version, same as above if the
                // OS_RELEASE_FILE does not exist.
                auto id = iter.path().native().substr(BMC_RO_PREFIX_LEN);
                ItemUpdater::erase(id);

                continue;
            }

            auto id = VersionClass::getId(version);

            // Check if the id has already been added. This can happen if the
            // BMC partitions / devices were manually flashed with the same
            // image.
            if (versions.find(id) != versions.end())
            {
                continue;
            }

            auto purpose = server::Version::VersionPurpose::BMC;
            restorePurpose(id, purpose);

            auto path = fs::path(SOFTWARE_OBJPATH) / id;

            // Create functional association if this is the functional
            // version
            if (version.compare(functionalVersion) == 0)
            {
                createFunctionalAssociation(path);
            }

            AssociationList associations = {};

            if (activationState == server::Activation::Activations::Active)
            {
                // Create an association to the BMC inventory item
                associations.emplace_back(std::make_tuple(
                    ACTIVATION_FWD_ASSOCIATION, ACTIVATION_REV_ASSOCIATION,
                    bmcInventoryPath));

                // Create an active association since this image is active
                createActiveAssociation(path);
            }

            // All updateable firmware components must expose the updateable
            // association.
            createUpdateableAssociation(path);

            // Create Version instance for this version.
            auto versionPtr = std::make_unique<VersionClass>(
                bus, path, version, purpose, "",
                std::bind(&ItemUpdater::erase, this, std::placeholders::_1));
            auto isVersionFunctional = versionPtr->isFunctional();
            if (!isVersionFunctional)
            {
                versionPtr->deleteObject =
                    std::make_unique<phosphor::software::manager::Delete>(
                        bus, path, *versionPtr);
            }
            versions.insert(std::make_pair(id, std::move(versionPtr)));

            // Create Activation instance for this version.
            activations.insert(std::make_pair(
                id, std::make_unique<Activation>(
                        bus, path, *this, id, activationState, associations)));

            // If Active, create RedundancyPriority instance for this
            // version.
            if (activationState == server::Activation::Activations::Active)
            {
                uint8_t priority = std::numeric_limits<uint8_t>::max();
                if (!restorePriority(id, priority))
                {
                    if (isVersionFunctional)
                    {
                        priority = 0;
                    }
                    else
                    {
                        log<level::ERR>("Unable to restore priority from file.",
                                        entry("VERSIONID=%s", id.c_str()));
                    }
                }
                activations.find(id)->second->redundancyPriority =
                    std::make_unique<RedundancyPriority>(
                        bus, path, *(activations.find(id)->second), priority,
                        false);
            }
        }
    }

    // If there are no bmc versions mounted under MEDIA_DIR, then read the
    // /etc/os-release and create rofs-<versionId> under MEDIA_DIR, then call
    // again processBMCImage() to create the D-Bus interface for it.
    if (activations.size() == 0)
    {
        auto version = VersionClass::getBMCVersion(OS_RELEASE_FILE);
        auto id = phosphor::software::manager::Version::getId(version);
        auto versionFileDir = BMC_ROFS_PREFIX + id + "/etc/";
        try
        {
            if (!fs::is_directory(versionFileDir))
            {
                fs::create_directories(versionFileDir);
            }
            auto versionFilePath = BMC_ROFS_PREFIX + id + OS_RELEASE_FILE;
            fs::create_directory_symlink(OS_RELEASE_FILE, versionFilePath);
            ItemUpdater::processBMCImage();
        }
        catch (const std::exception& e)
        {
            log<level::ERR>(e.what());
        }
    }

    mirrorUbootToAlt();
    return;
}

void ItemUpdater::erase(std::string entryId)
{
    // Find entry in versions map
    auto it = versions.find(entryId);
    if (it != versions.end())
    {
        if (it->second->isFunctional() && ACTIVE_BMC_MAX_ALLOWED > 1)
        {
            log<level::ERR>("Error: Version is currently running on the BMC. "
                            "Unable to remove.",
                            entry("VERSIONID=%s", entryId.c_str()));
            return;
        }
    }

    // First call resetUbootEnvVars() so that the BMC points to a valid image to
    // boot from. If resetUbootEnvVars() is called after the image is actually
    // deleted from the BMC flash, there'd be a time window where the BMC would
    // be pointing to a non-existent image to boot from.
    // Need to remove the entries from the activations map before that call so
    // that resetUbootEnvVars() doesn't use the version to be deleted.
    auto iteratorActivations = activations.find(entryId);
    if (iteratorActivations == activations.end())
    {
        log<level::ERR>("Error: Failed to find version in item updater "
                        "activations map. Unable to remove.",
                        entry("VERSIONID=%s", entryId.c_str()));
    }
    else
    {
        removeAssociations(iteratorActivations->second->path);
        this->activations.erase(entryId);
    }
    ItemUpdater::resetUbootEnvVars();

    if (it != versions.end())
    {
        // Delete ReadOnly partitions if it's not active
        removeReadOnlyPartition(entryId);
        removePersistDataDirectory(entryId);

        // Removing entry in versions map
        this->versions.erase(entryId);
    }
    else
    {
        // Delete ReadOnly partitions even if we can't find the version
        removeReadOnlyPartition(entryId);
        removePersistDataDirectory(entryId);

        log<level::ERR>("Error: Failed to find version in item updater "
                        "versions map. Unable to remove.",
                        entry("VERSIONID=%s", entryId.c_str()));
    }

    helper.clearEntry(entryId);

    return;
}

void ItemUpdater::deleteAll()
{
    std::vector<std::string> deletableVersions;

    for (const auto& versionIt : versions)
    {
        if (!versionIt.second->isFunctional())
        {
            deletableVersions.push_back(versionIt.first);
        }
    }

    for (const auto& deletableIt : deletableVersions)
    {
        ItemUpdater::erase(deletableIt);
    }

    helper.cleanup();
}

ItemUpdater::ActivationStatus
    ItemUpdater::validateSquashFSImage(const std::string& filePath)
{
    bool invalid = false;

    for (auto& bmcImage : bmcImages)
    {
        fs::path file(filePath);
        file /= bmcImage;
        std::ifstream efile(file.c_str());
        if (efile.good() != 1)
        {
            log<level::ERR>("Failed to find the BMC image.",
                            entry("IMAGE=%s", bmcImage.c_str()));
            invalid = true;
        }
    }

    if (invalid)
    {
        return ItemUpdater::ActivationStatus::invalid;
    }

    return ItemUpdater::ActivationStatus::ready;
}

void ItemUpdater::savePriority(const std::string& versionId, uint8_t value)
{
    storePriority(versionId, value);
    helper.setEntry(versionId, value);
}

void ItemUpdater::freePriority(uint8_t value, const std::string& versionId)
{
    std::map<std::string, uint8_t> priorityMap;

    // Insert the requested version and priority, it may not exist yet.
    priorityMap.insert(std::make_pair(versionId, value));

    for (const auto& intf : activations)
    {
        if (intf.second->redundancyPriority)
        {
            priorityMap.insert(std::make_pair(
                intf.first, intf.second->redundancyPriority.get()->priority()));
        }
    }

    // Lambda function to compare 2 priority values, use <= to allow duplicates
    typedef std::function<bool(std::pair<std::string, uint8_t>,
                               std::pair<std::string, uint8_t>)>
        cmpPriority;
    cmpPriority cmpPriorityFunc =
        [](std::pair<std::string, uint8_t> priority1,
           std::pair<std::string, uint8_t> priority2) {
            return priority1.second <= priority2.second;
        };

    // Sort versions by ascending priority
    std::set<std::pair<std::string, uint8_t>, cmpPriority> prioritySet(
        priorityMap.begin(), priorityMap.end(), cmpPriorityFunc);

    auto freePriorityValue = value;
    for (auto& element : prioritySet)
    {
        if (element.first == versionId)
        {
            continue;
        }
        if (element.second == freePriorityValue)
        {
            ++freePriorityValue;
            auto it = activations.find(element.first);
            it->second->redundancyPriority.get()->sdbusPriority(
                freePriorityValue);
        }
    }

    auto lowestVersion = prioritySet.begin()->first;
    if (value == prioritySet.begin()->second)
    {
        lowestVersion = versionId;
    }
    updateUbootEnvVars(lowestVersion);
}

void ItemUpdater::reset()
{
    constexpr auto setFactoryResetWait = std::chrono::seconds(3);
    helper.factoryReset();

    // Need to wait for env variables to complete, otherwise an immediate reboot
    // will not factory reset.
    std::this_thread::sleep_for(setFactoryResetWait);

    log<level::INFO>("BMC factory reset will take effect upon reboot.");
}

void ItemUpdater::removeReadOnlyPartition(std::string versionId)
{
    helper.removeVersion(versionId);
}

bool ItemUpdater::fieldModeEnabled(bool value)
{
    // enabling field mode is intended to be one way: false -> true
    if (value && !control::FieldMode::fieldModeEnabled())
    {
        control::FieldMode::fieldModeEnabled(value);

        auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                          SYSTEMD_INTERFACE, "StartUnit");
        method.append("obmc-flash-bmc-setenv@fieldmode\\x3dtrue.service",
                      "replace");
        bus.call_noreply(method);

        method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                     SYSTEMD_INTERFACE, "StopUnit");
        method.append("usr-local.mount", "replace");
        bus.call_noreply(method);

        std::vector<std::string> usrLocal = {"usr-local.mount"};

        method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                     SYSTEMD_INTERFACE, "MaskUnitFiles");
        method.append(usrLocal, false, true);
        bus.call_noreply(method);
    }
    else if (!value && control::FieldMode::fieldModeEnabled())
    {
        elog<NotAllowed>(xyz::openbmc_project::Common::NotAllowed::REASON(
            "FieldMode is not allowed to be cleared"));
    }

    return control::FieldMode::fieldModeEnabled();
}

void ItemUpdater::restoreFieldModeStatus()
{
    std::ifstream input("/dev/mtd/u-boot-env");
    std::string envVar;
    std::getline(input, envVar);

    if (envVar.find("fieldmode=true") != std::string::npos)
    {
        ItemUpdater::fieldModeEnabled(true);
    }
}

void ItemUpdater::setBMCInventoryPath()
{
    auto depth = 0;
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                          MAPPER_INTERFACE, "GetSubTreePaths");

    mapperCall.append(INVENTORY_PATH);
    mapperCall.append(depth);
    std::vector<std::string> filter = {BMC_INVENTORY_INTERFACE};
    mapperCall.append(filter);

    try
    {
        auto response = bus.call(mapperCall);

        using ObjectPaths = std::vector<std::string>;
        ObjectPaths result;
        response.read(result);

        if (!result.empty())
        {
            bmcInventoryPath = result.front();
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in mapper GetSubTreePath");
        return;
    }

    return;
}

void ItemUpdater::createActiveAssociation(const std::string& path)
{
    assocs.emplace_back(
        std::make_tuple(ACTIVE_FWD_ASSOCIATION, ACTIVE_REV_ASSOCIATION, path));
    associations(assocs);
}

void ItemUpdater::createFunctionalAssociation(const std::string& path)
{
    assocs.emplace_back(std::make_tuple(FUNCTIONAL_FWD_ASSOCIATION,
                                        FUNCTIONAL_REV_ASSOCIATION, path));
    associations(assocs);
}

void ItemUpdater::createUpdateableAssociation(const std::string& path)
{
    assocs.emplace_back(std::make_tuple(UPDATEABLE_FWD_ASSOCIATION,
                                        UPDATEABLE_REV_ASSOCIATION, path));
    associations(assocs);
}

void ItemUpdater::removeAssociations(const std::string& path)
{
    for (auto iter = assocs.begin(); iter != assocs.end();)
    {
        if ((std::get<2>(*iter)).compare(path) == 0)
        {
            iter = assocs.erase(iter);
            associations(assocs);
        }
        else
        {
            ++iter;
        }
    }
}

bool ItemUpdater::isLowestPriority(uint8_t value)
{
    for (const auto& intf : activations)
    {
        if (intf.second->redundancyPriority)
        {
            if (intf.second->redundancyPriority.get()->priority() < value)
            {
                return false;
            }
        }
    }
    return true;
}

void ItemUpdater::updateUbootEnvVars(const std::string& versionId)
{
    helper.updateUbootVersionId(versionId);
}

void ItemUpdater::resetUbootEnvVars()
{
    decltype(activations.begin()->second->redundancyPriority.get()->priority())
        lowestPriority = std::numeric_limits<uint8_t>::max();
    decltype(activations.begin()->second->versionId) lowestPriorityVersion;
    for (const auto& intf : activations)
    {
        if (!intf.second->redundancyPriority.get())
        {
            // Skip this version if the redundancyPriority is not initialized.
            continue;
        }

        if (intf.second->redundancyPriority.get()->priority() <= lowestPriority)
        {
            lowestPriority = intf.second->redundancyPriority.get()->priority();
            lowestPriorityVersion = intf.second->versionId;
        }
    }

    // Update the U-boot environment variable to point to the lowest priority
    updateUbootEnvVars(lowestPriorityVersion);
}

void ItemUpdater::freeSpace(Activation& caller)
{
    //  Versions with the highest priority in front
    std::priority_queue<std::pair<int, std::string>,
                        std::vector<std::pair<int, std::string>>,
                        std::less<std::pair<int, std::string>>>
        versionsPQ;

    std::size_t count = 0;
    for (const auto& iter : activations)
    {
        if ((iter.second.get()->activation() ==
             server::Activation::Activations::Active) ||
            (iter.second.get()->activation() ==
             server::Activation::Activations::Failed))
        {
            count++;
            // Don't put the functional version on the queue since we can't
            // remove the "running" BMC version.
            // If ACTIVE_BMC_MAX_ALLOWED <= 1, there is only one active BMC,
            // so remove functional version as well.
            // Don't delete the the Activation object that called this function.
            if ((versions.find(iter.second->versionId)
                     ->second->isFunctional() &&
                 ACTIVE_BMC_MAX_ALLOWED > 1) ||
                (iter.second->versionId == caller.versionId))
            {
                continue;
            }

            // Failed activations don't have priority, assign them a large value
            // for sorting purposes.
            auto priority = 999;
            if (iter.second.get()->activation() ==
                server::Activation::Activations::Active)
            {
                priority = iter.second->redundancyPriority.get()->priority();
            }

            versionsPQ.push(std::make_pair(priority, iter.second->versionId));
        }
    }

    // If the number of BMC versions is over ACTIVE_BMC_MAX_ALLOWED -1,
    // remove the highest priority one(s).
    while ((count >= ACTIVE_BMC_MAX_ALLOWED) && (!versionsPQ.empty()))
    {
        erase(versionsPQ.top().second);
        versionsPQ.pop();
        count--;
    }
}

void ItemUpdater::mirrorUbootToAlt()
{
    helper.mirrorAlt();
}

} // namespace updater
} // namespace software
} // namespace phosphor
