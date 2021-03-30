#pragma once

#include "config.h"

#include "settings.hpp"
#include "xyz/openbmc_project/State/Host/server.hpp"

#include <cereal/access.hpp>
#include <cereal/cereal.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Control/Boot/RebootAttempts/server.hpp>
#include <xyz/openbmc_project/State/Boot/Progress/server.hpp>
#include <xyz/openbmc_project/State/OperatingSystem/Status/server.hpp>

#include <experimental/filesystem>
#include <functional>
#include <string>

namespace phosphor
{
namespace state
{
namespace manager
{

using HostInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::server::Host,
    sdbusplus::xyz::openbmc_project::State::Boot::server::Progress,
    sdbusplus::xyz::openbmc_project::Control::Boot::server::RebootAttempts,
    sdbusplus::xyz::openbmc_project::State::OperatingSystem::server::Status>;

using namespace phosphor::logging;

namespace sdbusRule = sdbusplus::bus::match::rules;
namespace fs = std::experimental::filesystem;

/** @class Host
 *  @brief OpenBMC host state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Host
 *  DBus API.
 */
class Host : public HostInherit
{
  public:
    /** @brief Constructs Host State Manager
     *
     * @note This constructor passes 'true' to the base class in order to
     *       defer dbus object registration until we can run
     *       determineInitialState() and set our properties
     *
     * @param[in] bus       - The Dbus bus object
     * @param[in] objPath   - The Dbus object path
     */
    Host(sdbusplus::bus::bus& bus, const char* objPath) :
        HostInherit(bus, objPath, true), bus(bus),
        systemdSignalJobRemoved(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                sdbusRule::path("/org/freedesktop/systemd1") +
                sdbusRule::interface("org.freedesktop.systemd1.Manager"),
            std::bind(std::mem_fn(&Host::sysStateChangeJobRemoved), this,
                      std::placeholders::_1)),
        systemdSignalJobNew(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobNew") +
                sdbusRule::path("/org/freedesktop/systemd1") +
                sdbusRule::interface("org.freedesktop.systemd1.Manager"),
            std::bind(std::mem_fn(&Host::sysStateChangeJobNew), this,
                      std::placeholders::_1)),
        settings(bus)
    {
        // Enable systemd signals
        subscribeToSystemdSignals();

        // Will throw exception on fail
        determineInitialState();

        attemptsLeft(BOOT_COUNT_MAX_ALLOWED);

        // We deferred this until we could get our property correct
        this->emit_object_added();
    }

    /** @brief Set value of HostTransition */
    Transition requestedHostTransition(Transition value) override;

    /** @brief Set Value for boot progress */
    ProgressStages bootProgress(ProgressStages value) override;

    /** @brief Set Value for Operating System Status */
    OSStatus operatingSystemState(OSStatus value) override;

    /** @brief Set value of CurrentHostState */
    HostState currentHostState(HostState value) override;

    /**
     * @brief Set host reboot count to default
     *
     * OpenBMC software controls the number of allowed reboot attempts so
     * any external set request of this property will be overridden by
     * this function and set to the default.
     *
     * The only code responsible for decrementing the boot count resides
     * within this process and that will use the sub class interface
     * directly
     *
     * @param[in] value      - Reboot count value, will be ignored
     *
     * @return Default number of reboot attempts left
     */
    uint32_t attemptsLeft(uint32_t value) override
    {
        // value is ignored in this implementation
        (void)(value);
        log<level::DEBUG>("External request to reset reboot count");
        return (sdbusplus::xyz::openbmc_project::Control::Boot::server::
                    RebootAttempts::attemptsLeft(BOOT_COUNT_MAX_ALLOWED));
    }

  private:
    /**
     * @brief subscribe to the systemd signals
     *
     * This object needs to capture when it's systemd targets complete
     * so it can keep it's state updated
     *
     **/
    void subscribeToSystemdSignals();

    /**
     * @brief Determine initial host state and set internally
     *
     * @return Will throw exceptions on failure
     **/
    void determineInitialState();

    /** @brief Execute the transition request
     *
     * This function assumes the state has been validated and the host
     * is in an appropriate state for the transition to be started.
     *
     * @param[in] tranReq    - Transition requested
     */
    void executeTransition(Transition tranReq);

    /**
     * @brief Determine if target is active
     *
     * This function determines if the target is active and
     * helps prevent misleading log recorded states.
     *
     * @param[in] target - Target string to check on
     *
     * @return boolean corresponding to state active
     **/
    bool stateActive(const std::string& target);

    /**
     * @brief Determine if auto reboot flag is set
     *
     * @return boolean corresponding to current auto_reboot setting
     **/
    bool isAutoReboot();

    /** @brief Check if systemd state change is relevant to this object
     *
     * Instance specific interface to handle the detected systemd state
     * change
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    void sysStateChangeJobRemoved(sdbusplus::message::message& msg);

    /** @brief Check if JobNew systemd signal is relevant to this object
     *
     * In certain instances phosphor-state-manager needs to monitor for the
     * entry into a systemd target. This function will be used for these cases.
     *
     * Instance specific interface to handle the detected systemd state
     * change
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    void sysStateChangeJobNew(sdbusplus::message::message& msg);

    /** @brief Decrement reboot count
     *
     * This is used internally to this application to decrement the boot
     * count on each boot attempt. The host will use the external
     * attemptsLeft() interface to reset the count when a boot is successful
     *
     * @return number of reboot count attempts left
     */
    uint32_t decrementRebootCount();

    // Allow cereal class access to allow these next two function to be
    // private
    friend class cereal::access;

    /** @brief Function required by Cereal to perform serialization.
     *
     *  @tparam Archive - Cereal archive type (binary in our case).
     *  @param[in] archive - reference to Cereal archive.
     *  @param[in] version - Class version that enables handling
     *                       a serialized data across code levels
     */
    template <class Archive>
    void save(Archive& archive, const std::uint32_t version) const
    {
        // version is not used currently
        (void)(version);
        archive(convertForMessage(sdbusplus::xyz::openbmc_project::State::
                                      server::Host::requestedHostTransition()),
                convertForMessage(sdbusplus::xyz::openbmc_project::State::Boot::
                                      server::Progress::bootProgress()),
                convertForMessage(
                    sdbusplus::xyz::openbmc_project::State::OperatingSystem::
                        server::Status::operatingSystemState()));
    }

    /** @brief Function required by Cereal to perform deserialization.
     *
     *  @tparam Archive - Cereal archive type (binary in our case).
     *  @param[in] archive - reference to Cereal archive.
     *  @param[in] version - Class version that enables handling
     *                       a serialized data across code levels
     */
    template <class Archive>
    void load(Archive& archive, const std::uint32_t version)
    {
        // version is not used currently
        (void)(version);
        std::string reqTranState;
        std::string bootProgress;
        std::string osState;
        archive(reqTranState, bootProgress, osState);
        auto reqTran = Host::convertTransitionFromString(reqTranState);
        // When restoring, set the requested state with persistent value
        // but don't call the override which would execute it
        sdbusplus::xyz::openbmc_project::State::server::Host::
            requestedHostTransition(reqTran);
        sdbusplus::xyz::openbmc_project::State::Boot::server::Progress::
            bootProgress(Host::convertProgressStagesFromString(bootProgress));
        sdbusplus::xyz::openbmc_project::State::OperatingSystem::server::
            Status::operatingSystemState(
                Host::convertOSStatusFromString(osState));
    }

    /** @brief Serialize and persist requested host state
     *
     *  @param[in] dir - pathname of file where the serialized host state will
     *                   be placed.
     *
     *  @return fs::path - pathname of persisted requested host state.
     */
    fs::path serialize(const fs::path& dir = fs::path(HOST_STATE_PERSIST_PATH));

    /** @brief Deserialze a persisted requested host state.
     *
     *  @param[in] path - pathname of persisted host state file
     *
     *  @return bool - true if the deserialization was successful, false
     *                 otherwise.
     */
    bool deserialize(const fs::path& path);

    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief Used to subscribe to dbus systemd JobRemoved signal **/
    sdbusplus::bus::match_t systemdSignalJobRemoved;

    /** @brief Used to subscribe to dbus systemd JobNew signal **/
    sdbusplus::bus::match_t systemdSignalJobNew;

    // Settings objects of interest
    settings::Objects settings;
};

} // namespace manager
} // namespace state
} // namespace phosphor
