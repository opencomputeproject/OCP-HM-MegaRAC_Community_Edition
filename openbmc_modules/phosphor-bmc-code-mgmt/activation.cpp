#include "activation.hpp"

#include "images.hpp"
#include "item_updater.hpp"
#include "serialize.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#ifdef WANT_SIGNATURE_VERIFY
#include "image_verify.hpp"
#endif

namespace phosphor
{
namespace software
{
namespace updater
{

namespace softwareServer = sdbusplus::xyz::openbmc_project::Software::server;

using namespace phosphor::logging;
using sdbusplus::exception::SdBusError;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

#ifdef WANT_SIGNATURE_VERIFY
namespace control = sdbusplus::xyz::openbmc_project::Control::server;
#endif

void Activation::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                            SYSTEMD_INTERFACE, "Subscribe");
    try
    {
        this->bus.call_noreply(method);
    }
    catch (const SdBusError& e)
    {
        if (e.name() != nullptr &&
            strcmp("org.freedesktop.systemd1.AlreadySubscribed", e.name()) == 0)
        {
            // If an Activation attempt fails, the Unsubscribe method is not
            // called. This may lead to an AlreadySubscribed error if the
            // Activation is re-attempted.
        }
        else
        {
            log<level::ERR>("Error subscribing to systemd",
                            entry("ERROR=%s", e.what()));
        }
    }

    return;
}

void Activation::unsubscribeFromSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                            SYSTEMD_INTERFACE, "Unsubscribe");
    try
    {
        this->bus.call_noreply(method);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in unsubscribing from systemd signals",
                        entry("ERROR=%s", e.what()));
    }

    return;
}

auto Activation::activation(Activations value) -> Activations
{

    if ((value != softwareServer::Activation::Activations::Active) &&
        (value != softwareServer::Activation::Activations::Activating))
    {
        redundancyPriority.reset(nullptr);
    }

    if (value == softwareServer::Activation::Activations::Activating)
    {

#ifdef HOST_BIOS_UPGRADE
        auto purpose = parent.versions.find(versionId)->second->purpose();
        if (purpose == VersionPurpose::Host)
        {
            if (!activationProgress)
            {
                activationProgress =
                    std::make_unique<ActivationProgress>(bus, path);
            }

            // Enable systemd signals
            subscribeToSystemdSignals();

            // Set initial progress
            activationProgress->progress(20);

            // Initiate image writing to flash
            flashWriteHost();

            return softwareServer::Activation::activation(value);
        }
#endif

#ifdef WANT_SIGNATURE_VERIFY
        fs::path uploadDir(IMG_UPLOAD_DIR);
        if (!verifySignature(uploadDir / versionId, SIGNED_IMAGE_CONF_PATH))
        {
            onVerifyFailed();
            // Stop the activation process, if fieldMode is enabled.
            if (parent.control::FieldMode::fieldModeEnabled())
            {
                return softwareServer::Activation::activation(
                    softwareServer::Activation::Activations::Failed);
            }
        }
#endif

        if (!activationProgress)
        {
            activationProgress =
                std::make_unique<ActivationProgress>(bus, path);
        }

        if (!activationBlocksTransition)
        {
            activationBlocksTransition =
                std::make_unique<ActivationBlocksTransition>(bus, path);
        }

        activationProgress->progress(10);

        parent.freeSpace(*this);

        // Enable systemd signals
        Activation::subscribeToSystemdSignals();

        flashWrite();

#if defined UBIFS_LAYOUT || defined MMC_LAYOUT

        return softwareServer::Activation::activation(value);

#else // STATIC_LAYOUT

        onFlashWriteSuccess();
        return softwareServer::Activation::activation(
            softwareServer::Activation::Activations::Active);
#endif
    }
    else
    {
        activationBlocksTransition.reset(nullptr);
        activationProgress.reset(nullptr);
    }
    return softwareServer::Activation::activation(value);
}

void Activation::onFlashWriteSuccess()
{
    activationProgress->progress(100);

    activationBlocksTransition.reset(nullptr);
    activationProgress.reset(nullptr);

    rwVolumeCreated = false;
    roVolumeCreated = false;
    ubootEnvVarsUpdated = false;
    Activation::unsubscribeFromSystemdSignals();

    storePurpose(versionId, parent.versions.find(versionId)->second->purpose());

    if (!redundancyPriority)
    {
        redundancyPriority =
            std::make_unique<RedundancyPriority>(bus, path, *this, 0);
    }

    // Remove version object from image manager
    Activation::deleteImageManagerObject();

    // Create active association
    parent.createActiveAssociation(path);

    // Create updateable association as this
    // can be re-programmed.
    parent.createUpdateableAssociation(path);

    if (Activation::checkApplyTimeImmediate() == true)
    {
        log<level::INFO>("Image Active. ApplyTime is immediate, "
                         "rebooting BMC.");
        Activation::rebootBmc();
    }
    else
    {
        log<level::INFO>("BMC image ready, need reboot to get activated.");
    }

    activation(softwareServer::Activation::Activations::Active);
}

void Activation::deleteImageManagerObject()
{
    // Call the Delete object for <versionID> inside image_manager
    auto method = this->bus.new_method_call(VERSION_BUSNAME, path.c_str(),
                                            "xyz.openbmc_project.Object.Delete",
                                            "Delete");
    try
    {
        bus.call_noreply(method);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in Deleting image from image manager",
                        entry("VERSIONPATH=%s", path.c_str()));
        return;
    }
}

auto Activation::requestedActivation(RequestedActivations value)
    -> RequestedActivations
{
    rwVolumeCreated = false;
    roVolumeCreated = false;
    ubootEnvVarsUpdated = false;

    if ((value == softwareServer::Activation::RequestedActivations::Active) &&
        (softwareServer::Activation::requestedActivation() !=
         softwareServer::Activation::RequestedActivations::Active))
    {
        if ((softwareServer::Activation::activation() ==
             softwareServer::Activation::Activations::Ready) ||
            (softwareServer::Activation::activation() ==
             softwareServer::Activation::Activations::Failed))
        {
            Activation::activation(
                softwareServer::Activation::Activations::Activating);
        }
    }
    return softwareServer::Activation::requestedActivation(value);
}

uint8_t RedundancyPriority::priority(uint8_t value)
{
    // Set the priority value so that the freePriority() function can order
    // the versions by priority.
    auto newPriority = softwareServer::RedundancyPriority::priority(value);
    parent.parent.savePriority(parent.versionId, value);
    parent.parent.freePriority(value, parent.versionId);
    return newPriority;
}

uint8_t RedundancyPriority::sdbusPriority(uint8_t value)
{
    parent.parent.savePriority(parent.versionId, value);
    return softwareServer::RedundancyPriority::priority(value);
}

void Activation::unitStateChange(sdbusplus::message::message& msg)
{
    if (softwareServer::Activation::activation() !=
        softwareServer::Activation::Activations::Activating)
    {
        return;
    }

#ifdef HOST_BIOS_UPGRADE
    auto purpose = parent.versions.find(versionId)->second->purpose();
    if (purpose == VersionPurpose::Host)
    {
        onStateChangesBios(msg);
        return;
    }
#endif

    onStateChanges(msg);

    return;
}

#ifdef WANT_SIGNATURE_VERIFY
bool Activation::verifySignature(const fs::path& imageDir,
                                 const fs::path& confDir)
{
    using Signature = phosphor::software::image::Signature;

    Signature signature(imageDir, confDir);

    return signature.verify();
}

void Activation::onVerifyFailed()
{
    log<level::ERR>("Error occurred during image validation");
    report<InternalFailure>();
}
#endif

void ActivationBlocksTransition::enableRebootGuard()
{
    log<level::INFO>("BMC image activating - BMC reboots are disabled.");

    auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append("reboot-guard-enable.service", "replace");
    bus.call_noreply(method);
}

void ActivationBlocksTransition::disableRebootGuard()
{
    log<level::INFO>("BMC activation has ended - BMC reboots are re-enabled.");

    auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append("reboot-guard-disable.service", "replace");
    bus.call_noreply(method);
}

bool Activation::checkApplyTimeImmediate()
{
    auto service = utils::getService(bus, applyTimeObjPath, applyTimeIntf);
    if (service.empty())
    {
        log<level::INFO>("Error getting the service name for BMC image "
                         "ApplyTime. The BMC needs to be manually rebooted to "
                         "complete the image activation if needed "
                         "immediately.");
    }
    else
    {

        auto method = bus.new_method_call(service.c_str(), applyTimeObjPath,
                                          dbusPropIntf, "Get");
        method.append(applyTimeIntf, applyTimeProp);

        try
        {
            auto reply = bus.call(method);

            std::variant<std::string> result;
            reply.read(result);
            auto applyTime = std::get<std::string>(result);
            if (applyTime == applyTimeImmediate)
            {
                return true;
            }
        }
        catch (const SdBusError& e)
        {
            log<level::ERR>("Error in getting ApplyTime",
                            entry("ERROR=%s", e.what()));
        }
    }
    return false;
}

#ifdef HOST_BIOS_UPGRADE
void Activation::flashWriteHost()
{
    auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    auto biosServiceFile = "obmc-flash-host-bios@" + versionId + ".service";
    method.append(biosServiceFile, "replace");
    try
    {
        auto reply = bus.call(method);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in trying to upgrade Host Bios.");
        report<InternalFailure>();
    }
}

void Activation::onStateChangesBios(sdbusplus::message::message& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    auto biosServiceFile = "obmc-flash-host-bios@" + versionId + ".service";

    if (newStateUnit == biosServiceFile)
    {
        // unsubscribe to systemd signals
        unsubscribeFromSystemdSignals();

        // Remove version object from image manager
        deleteImageManagerObject();

        if (newStateResult == "done")
        {
            // Set activation progress to 100
            activationProgress->progress(100);

            // Set Activation value to active
            activation(softwareServer::Activation::Activations::Active);

            log<level::INFO>("Bios upgrade completed successfully.");
        }
        else if (newStateResult == "failed")
        {
            // Set Activation value to Failed
            activation(softwareServer::Activation::Activations::Failed);

            log<level::ERR>("Bios upgrade failed.");
        }
    }

    return;
}

#endif

void Activation::rebootBmc()
{
    auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append("force-reboot.service", "replace");
    try
    {
        auto reply = bus.call(method);
    }
    catch (const SdBusError& e)
    {
        log<level::ALERT>("Error in trying to reboot the BMC. "
                          "The BMC needs to be manually rebooted to complete "
                          "the image activation.");
        report<InternalFailure>();
    }
}

} // namespace updater
} // namespace software
} // namespace phosphor
