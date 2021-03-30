#include "config.h"

#include "activation.hpp"

namespace phosphor
{
namespace software
{
namespace updater
{

namespace softwareServer = sdbusplus::xyz::openbmc_project::Software::server;

void Activation::flashWrite()
{
    auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                      SYSTEMD_INTERFACE, "StartUnit");
    method.append("obmc-flash-bmc-ubirw.service", "replace");
    bus.call_noreply(method);

    auto roServiceFile = "obmc-flash-bmc-ubiro@" + versionId + ".service";
    method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                 SYSTEMD_INTERFACE, "StartUnit");
    method.append(roServiceFile, "replace");
    bus.call_noreply(method);

    return;
}

void Activation::onStateChanges(sdbusplus::message::message& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    auto rwServiceFile = "obmc-flash-bmc-ubirw.service";
    auto roServiceFile = "obmc-flash-bmc-ubiro@" + versionId + ".service";
    auto ubootVarsServiceFile =
        "obmc-flash-bmc-updateubootvars@" + versionId + ".service";

    if (newStateUnit == rwServiceFile && newStateResult == "done")
    {
        rwVolumeCreated = true;
        activationProgress->progress(activationProgress->progress() + 20);
    }

    if (newStateUnit == roServiceFile && newStateResult == "done")
    {
        roVolumeCreated = true;
        activationProgress->progress(activationProgress->progress() + 50);
    }

    if (newStateUnit == ubootVarsServiceFile && newStateResult == "done")
    {
        ubootEnvVarsUpdated = true;
    }

    if (newStateUnit == rwServiceFile || newStateUnit == roServiceFile ||
        newStateUnit == ubootVarsServiceFile)
    {
        if (newStateResult == "failed" || newStateResult == "dependency")
        {
            Activation::activation(
                softwareServer::Activation::Activations::Failed);
        }
        else if (rwVolumeCreated && roVolumeCreated) // Volumes were created
        {
            if (!ubootEnvVarsUpdated)
            {
                activationProgress->progress(90);

                // Set the priority which triggers the service that updates the
                // environment variables.
                if (!Activation::redundancyPriority)
                {
                    Activation::redundancyPriority =
                        std::make_unique<RedundancyPriority>(bus, path, *this,
                                                             0);
                }
            }
            else // Environment variables were updated
            {
                Activation::onFlashWriteSuccess();
            }
        }
    }

    return;
}

} // namespace updater
} // namespace software
} // namespace phosphor
