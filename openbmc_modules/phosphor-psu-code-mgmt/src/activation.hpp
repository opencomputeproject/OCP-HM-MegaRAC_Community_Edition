#pragma once

#include "config.h"

#include "activation_listener.hpp"
#include "association_interface.hpp"
#include "types.hpp"
#include "version.hpp"

#include <queue>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Association/Definitions/server.hpp>
#include <xyz/openbmc_project/Common/FilePath/server.hpp>
#include <xyz/openbmc_project/Software/Activation/server.hpp>
#include <xyz/openbmc_project/Software/ActivationBlocksTransition/server.hpp>
#include <xyz/openbmc_project/Software/ActivationProgress/server.hpp>
#include <xyz/openbmc_project/Software/ExtendedVersion/server.hpp>

class TestActivation;

namespace phosphor
{
namespace software
{
namespace updater
{

namespace sdbusRule = sdbusplus::bus::match::rules;

using ActivationBlocksTransitionInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Software::server::
        ActivationBlocksTransition>;

/** @class ActivationBlocksTransition
 *  @brief OpenBMC ActivationBlocksTransition implementation.
 *  @details A concrete implementation for
 *  xyz.openbmc_project.Software.ActivationBlocksTransition DBus API.
 */
class ActivationBlocksTransition : public ActivationBlocksTransitionInherit
{
  public:
    /** @brief Constructs ActivationBlocksTransition.
     *
     * @param[in] bus    - The Dbus bus object
     * @param[in] path   - The Dbus object path
     */
    ActivationBlocksTransition(sdbusplus::bus::bus& bus,
                               const std::string& path) :
        ActivationBlocksTransitionInherit(bus, path.c_str(),
                                          action::emit_interface_added),
        bus(bus)
    {
        enableRebootGuard();
    }

    ~ActivationBlocksTransition()
    {
        disableRebootGuard();
    }

  private:
    sdbusplus::bus::bus& bus;

    /** @brief Enables a Guard that blocks any BMC reboot commands */
    void enableRebootGuard();

    /** @brief Disables any guard that was blocking the BMC reboot */
    void disableRebootGuard();
};

using ActivationProgressInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Software::server::ActivationProgress>;

class ActivationProgress : public ActivationProgressInherit
{
  public:
    /** @brief Constructs ActivationProgress.
     *
     * @param[in] bus    - The Dbus bus object
     * @param[in] path   - The Dbus object path
     */
    ActivationProgress(sdbusplus::bus::bus& bus, const std::string& path) :
        ActivationProgressInherit(bus, path.c_str(),
                                  action::emit_interface_added)
    {
        progress(0);
    }
};

using ActivationInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Software::server::ExtendedVersion,
    sdbusplus::xyz::openbmc_project::Software::server::Activation,
    sdbusplus::xyz::openbmc_project::Association::server::Definitions,
    sdbusplus::xyz::openbmc_project::Common::server::FilePath>;

/** @class Activation
 *  @brief OpenBMC activation software management implementation.
 *  @details A concrete implementation for
 *  xyz.openbmc_project.Software.Activation DBus API.
 */
class Activation : public ActivationInherit
{
  public:
    friend class ::TestActivation;
    using Status = Activations;

    /** @brief Constructs Activation Software Manager
     *
     * @param[in] bus    - The Dbus bus object
     * @param[in] path   - The Dbus object path
     * @param[in] versionId  - The software version id
     * @param[in] extVersion - The extended version
     * @param[in] activationStatus - The status of Activation
     * @param[in] assocs - Association objects
     * @param[in] filePath - The image filesystem path
     */
    Activation(sdbusplus::bus::bus& bus, const std::string& objPath,
               const std::string& versionId, const std::string& extVersion,
               Status activationStatus, const AssociationList& assocs,
               const std::string& filePath,
               AssociationInterface* associationInterface,
               ActivationListener* activationListener) :
        ActivationInherit(bus, objPath.c_str(), true),
        bus(bus), objPath(objPath), versionId(versionId),
        systemdSignals(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                sdbusRule::path("/org/freedesktop/systemd1") +
                sdbusRule::interface("org.freedesktop.systemd1.Manager"),
            std::bind(&Activation::unitStateChange, this,
                      std::placeholders::_1)),
        associationInterface(associationInterface),
        activationListener(activationListener)
    {
        // Set Properties.
        extendedVersion(extVersion);
        activation(activationStatus);
        associations(assocs);
        path(filePath);

        auto info = Version::getExtVersionInfo(extVersion);
        manufacturer = info["manufacturer"];
        model = info["model"];

        // Emit deferred signal.
        emit_object_added();
    }

    /** @brief Overloaded Activation property setter function
     *
     * @param[in] value - One of Activation::Activations
     *
     * @return Success or exception thrown
     */
    Status activation(Status value) override;

    /** @brief Activation */
    using ActivationInherit::activation;

    /** @brief Overloaded requestedActivation property setter function
     *
     * @param[in] value - One of Activation::RequestedActivations
     *
     * @return Success or exception thrown
     */
    RequestedActivations
        requestedActivation(RequestedActivations value) override;

    /** @brief Get the object path */
    const std::string& getObjectPath() const
    {
        return objPath;
    }

    /** @brief Get the version ID */
    const std::string& getVersionId() const
    {
        return versionId;
    }

  private:
    /** @brief Check if systemd state change is relevant to this object
     *
     * Instance specific interface to handle the detected systemd state
     * change
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    void unitStateChange(sdbusplus::message::message& msg);

    /**
     * @brief Delete the version from Image Manager and the
     *        untar image from image upload dir.
     */
    void deleteImageManagerObject();

    /** @brief Invoke the update service for the PSU
     *
     * @param[in] psuInventoryPath - The PSU inventory to be updated.
     *
     * @return true if the update starts, and false if it fails.
     */
    bool doUpdate(const std::string& psuInventoryPath);

    /** @brief Do PSU update one-by-one
     *
     * @return true if the update starts, and false if it fails.
     */
    bool doUpdate();

    /** @brief Handle an update done event */
    void onUpdateDone();

    /** @brief Handle an update failure event */
    void onUpdateFailed();

    /** @brief Start PSU update */
    Status startActivation();

    /** @brief Finish PSU update */
    void finishActivation();

    /** @brief Check if the PSU is comaptible with this software*/
    bool isCompatible(const std::string& psuInventoryPath);

    /** @brief Store the updated PSU image to persistent dir */
    void storeImage();

    /** @brief Construct the systemd service name
     *
     * @param[in] psuInventoryPath - The PSU inventory to be updated.
     *
     * @return The escaped string of systemd unit to do the PSU update.
     */
    std::string getUpdateService(const std::string& psuInventoryPath);

    /** @brief Persistent sdbusplus DBus bus connection */
    sdbusplus::bus::bus& bus;

    /** @brief Persistent DBus object path */
    std::string objPath;

    /** @brief Version id */
    std::string versionId;

    /** @brief Used to subscribe to dbus systemd signals */
    sdbusplus::bus::match_t systemdSignals;

    /** @brief The queue of psu objects to be updated */
    std::queue<std::string> psuQueue;

    /** @brief The progress step for each PSU update is done */
    uint32_t progressStep;

    /** @brief The PSU update systemd unit */
    std::string psuUpdateUnit;

    /** @brief The PSU Inventory path of the current updating PSU */
    std::string currentUpdatingPsu;

    /** @brief Persistent ActivationBlocksTransition dbus object */
    std::unique_ptr<ActivationBlocksTransition> activationBlocksTransition;

    /** @brief Persistent ActivationProgress dbus object */
    std::unique_ptr<ActivationProgress> activationProgress;

    /** @brief The AssociationInterface pointer */
    AssociationInterface* associationInterface;

    /** @brief The activationListener pointer */
    ActivationListener* activationListener;

    /** @brief The PSU manufacturer of the software */
    std::string manufacturer;

    /** @brief The PSU model of the software */
    std::string model;
};

} // namespace updater
} // namespace software
} // namespace phosphor
