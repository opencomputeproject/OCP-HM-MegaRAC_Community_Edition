#include "config.h"

#include "activation.hpp"

#include "utils.hpp"

#include <cassert>
#include <filesystem>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace software
{
namespace updater
{

constexpr auto SYSTEMD_BUSNAME = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

namespace fs = std::filesystem;
namespace softwareServer = sdbusplus::xyz::openbmc_project::Software::server;

using namespace phosphor::logging;
using sdbusplus::exception::SdBusError;
using SoftwareActivation = softwareServer::Activation;

auto Activation::activation(Activations value) -> Activations
{
    if (value == Status::Activating)
    {
        value = startActivation();
    }
    else
    {
        activationBlocksTransition.reset();
        activationProgress.reset();
    }

    return SoftwareActivation::activation(value);
}

auto Activation::requestedActivation(RequestedActivations value)
    -> RequestedActivations
{
    if ((value == SoftwareActivation::RequestedActivations::Active) &&
        (SoftwareActivation::requestedActivation() !=
         SoftwareActivation::RequestedActivations::Active))
    {
        // PSU image could be activated even when it's in active,
        // e.g. in case a PSU is replaced and has a older image, it will be
        // updated with the running PSU image that is stored in BMC.
        if ((activation() == Status::Ready) ||
            (activation() == Status::Failed) || activation() == Status::Active)
        {
            activation(Status::Activating);
        }
    }
    return SoftwareActivation::requestedActivation(value);
}

void Activation::unitStateChange(sdbusplus::message::message& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    if (newStateUnit == psuUpdateUnit)
    {
        if (newStateResult == "done")
        {
            onUpdateDone();
        }
        if (newStateResult == "failed" || newStateResult == "dependency")
        {
            onUpdateFailed();
        }
    }
}

bool Activation::doUpdate(const std::string& psuInventoryPath)
{
    currentUpdatingPsu = psuInventoryPath;
    psuUpdateUnit = getUpdateService(currentUpdatingPsu);
    try
    {
        auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                          SYSTEMD_INTERFACE, "StartUnit");
        method.append(psuUpdateUnit, "replace");
        bus.call_noreply(method);
        return true;
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error staring service", entry("ERROR=%s", e.what()));
        onUpdateFailed();
        return false;
    }
}

bool Activation::doUpdate()
{
    // When the queue is empty, all updates are done
    if (psuQueue.empty())
    {
        finishActivation();
        return true;
    }

    // Do the update on a PSU
    const auto& psu = psuQueue.front();
    return doUpdate(psu);
}

void Activation::onUpdateDone()
{
    auto progress = activationProgress->progress() + progressStep;
    activationProgress->progress(progress);

    // Update the activation association
    auto assocs = associations();
    assocs.emplace_back(ACTIVATION_FWD_ASSOCIATION, ACTIVATION_REV_ASSOCIATION,
                        currentUpdatingPsu);
    associations(assocs);

    activationListener->onUpdateDone(versionId, currentUpdatingPsu);
    currentUpdatingPsu.clear();

    psuQueue.pop();
    doUpdate(); // Update the next psu
}

void Activation::onUpdateFailed()
{
    // TODO: report an event
    log<level::ERR>("Failed to udpate PSU",
                    entry("PSU=%s", psuQueue.front().c_str()));
    std::queue<std::string>().swap(psuQueue); // Clear the queue
    activation(Status::Failed);
}

Activation::Status Activation::startActivation()
{
    // Check if the activation has file path
    if (path().empty())
    {
        log<level::WARNING>("No image for the activation, skipped",
                            entry("VERSION_ID=%s", versionId.c_str()));
        return activation(); // Return the previous activation status
    }

    auto psuPaths = utils::getPSUInventoryPath(bus);
    if (psuPaths.empty())
    {
        log<level::WARNING>("No PSU inventory found");
        return Status::Failed;
    }

    for (const auto& p : psuPaths)
    {
        if (isCompatible(p))
        {
            if (utils::isAssociated(p, associations()))
            {
                log<level::NOTICE>("PSU already running the image, skipping",
                                   entry("PSU=%s", p.c_str()));
                continue;
            }
            psuQueue.push(p);
        }
        else
        {
            log<level::NOTICE>("PSU not compatible",
                               entry("PSU=%s", p.c_str()));
        }
    }

    if (psuQueue.empty())
    {
        log<level::WARNING>("No PSU compatible with the software");
        return activation(); // Return the previous activation status
    }

    if (!activationProgress)
    {
        activationProgress = std::make_unique<ActivationProgress>(bus, objPath);
    }
    if (!activationBlocksTransition)
    {
        activationBlocksTransition =
            std::make_unique<ActivationBlocksTransition>(bus, objPath);
    }

    // The progress to be increased for each successful update of PSU
    // E.g. in case we have 4 PSUs:
    //   1. Initial progrss is 10
    //   2. Add 20 after each update is done, so we will see progress to be 30,
    //      50, 70, 90
    //   3. When all PSUs are updated, it will be 100 and the interface is
    //   removed.
    progressStep = 80 / psuQueue.size();
    if (doUpdate())
    {
        activationProgress->progress(10);
        return Status::Activating;
    }
    else
    {
        return Status::Failed;
    }
}

void Activation::finishActivation()
{
    storeImage();
    activationProgress->progress(100);

    deleteImageManagerObject();

    associationInterface->createActiveAssociation(objPath);
    associationInterface->addFunctionalAssociation(objPath);
    associationInterface->addUpdateableAssociation(objPath);

    // Reset RequestedActivations to none so that it could be activated in
    // future
    requestedActivation(SoftwareActivation::RequestedActivations::None);
    activation(Status::Active);
}

void Activation::deleteImageManagerObject()
{
    // Get the Delete object for <versionID> inside image_manager
    constexpr auto versionServiceStr = "xyz.openbmc_project.Software.Version";
    constexpr auto deleteInterface = "xyz.openbmc_project.Object.Delete";
    std::string versionService;
    auto services = utils::getServices(bus, objPath.c_str(), deleteInterface);

    // We need to find the phosphor-version-software-manager's version service
    // to invoke the delete interface
    for (const auto& service : services)
    {
        if (service.find(versionServiceStr) != std::string::npos)
        {
            versionService = service;
            break;
        }
    }
    if (versionService.empty())
    {
        // When updating a stored image, there is no version object created by
        // "xyz.openbmc_project.Software.Version" service, so skip it.
        return;
    }

    // Call the Delete object for <versionID> inside image_manager
    auto method = bus.new_method_call(versionService.c_str(), objPath.c_str(),
                                      deleteInterface, "Delete");
    try
    {
        bus.call(method);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error performing call to Delete object path",
                        entry("ERROR=%s", e.what()),
                        entry("PATH=%s", objPath.c_str()));
    }
}

bool Activation::isCompatible(const std::string& psuInventoryPath)
{
    auto service =
        utils::getService(bus, psuInventoryPath.c_str(), ASSET_IFACE);
    auto psuManufacturer = utils::getProperty<std::string>(
        bus, service.c_str(), psuInventoryPath.c_str(), ASSET_IFACE,
        MANUFACTURER);
    auto psuModel = utils::getProperty<std::string>(
        bus, service.c_str(), psuInventoryPath.c_str(), ASSET_IFACE, MODEL);
    if (psuModel != model)
    {
        // The model shall match
        return false;
    }
    if (!psuManufacturer.empty())
    {
        // If PSU inventory has manufacturer property, it shall match
        return psuManufacturer == manufacturer;
    }
    return true;
}

void Activation::storeImage()
{
    // Store image in persistent dir separated by model
    // and only store the latest one by removing old ones
    auto src = path();
    auto dst = fs::path(IMG_DIR_PERSIST) / model;
    if (src == dst)
    {
        // This happens when updating an stored image, no need to store it again
        return;
    }
    try
    {
        fs::remove_all(dst);
        fs::create_directories(dst);
        fs::copy(src, dst);
        path(dst.string()); // Update the FilePath interface
    }
    catch (const fs::filesystem_error& e)
    {
        log<level::ERR>("Error storing PSU image", entry("ERROR=%s", e.what()),
                        entry("SRC=%s", src.c_str()),
                        entry("DST=%s", dst.c_str()));
    }
}

std::string Activation::getUpdateService(const std::string& psuInventoryPath)
{
    fs::path imagePath(path());

    // The systemd unit shall be escaped
    std::string args = psuInventoryPath;
    args += "\\x20";
    args += imagePath;
    std::replace(args.begin(), args.end(), '/', '-');

    std::string service = PSU_UPDATE_SERVICE;
    auto p = service.find('@');
    assert(p != std::string::npos);
    service.insert(p + 1, args);
    return service;
}

void ActivationBlocksTransition::enableRebootGuard()
{
    log<level::INFO>("PSU image activating - BMC reboots are disabled.");

    auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append("reboot-guard-enable.service", "replace");
    bus.call_noreply_noerror(method);
}

void ActivationBlocksTransition::disableRebootGuard()
{
    log<level::INFO>("PSU activation has ended - BMC reboots are re-enabled.");

    auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append("reboot-guard-disable.service", "replace");
    bus.call_noreply_noerror(method);
}

} // namespace updater
} // namespace software
} // namespace phosphor
