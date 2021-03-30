#pragma once

#include "config.h"

#include "xyz/openbmc_project/State/Chassis/server.hpp"
#include "xyz/openbmc_project/State/PowerOnHours/server.hpp"

#include <cereal/cereal.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <experimental/filesystem>
#include <functional>

namespace phosphor
{
namespace state
{
namespace manager
{

using ChassisInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::server::Chassis,
    sdbusplus::xyz::openbmc_project::State::server::PowerOnHours>;
namespace sdbusRule = sdbusplus::bus::match::rules;
namespace fs = std::experimental::filesystem;

/** @class Chassis
 *  @brief OpenBMC chassis state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.Chassis
 *  DBus API.
 */
class Chassis : public ChassisInherit
{
  public:
    /** @brief Constructs Chassis State Manager
     *
     * @note This constructor passes 'true' to the base class in order to
     *       defer dbus object registration until we can run
     *       determineInitialState() and set our properties
     *
     * @param[in] bus       - The Dbus bus object
     * @param[in] objPath   - The Dbus object path
     */
    Chassis(sdbusplus::bus::bus& bus, const char* objPath) :
        ChassisInherit(bus, objPath, true), bus(bus),
        systemdSignals(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                sdbusRule::path("/org/freedesktop/systemd1") +
                sdbusRule::interface("org.freedesktop.systemd1.Manager"),
            std::bind(std::mem_fn(&Chassis::sysStateChange), this,
                      std::placeholders::_1)),
        pOHTimer(sdeventplus::Event::get_default(),
                 std::bind(&Chassis::pOHCallback, this), std::chrono::hours{1},
                 std::chrono::minutes{1})
    {
        subscribeToSystemdSignals();

        restoreChassisStateChangeTime();

        determineInitialState();

        restorePOHCounter(); // restore POHCounter from persisted file

        // We deferred this until we could get our property correct
        this->emit_object_added();
    }

    /** @brief Set value of RequestedPowerTransition */
    Transition requestedPowerTransition(Transition value) override;

    /** @brief Set value of CurrentPowerState */
    PowerState currentPowerState(PowerState value) override;

    /** @brief Get value of POHCounter */
    using ChassisInherit::pOHCounter;

    /** @brief Increment POHCounter if Chassis Power state is ON */
    void startPOHCounter();

  private:
    /** @brief Determine initial chassis state and set internally */
    void determineInitialState();

    /**
     * @brief subscribe to the systemd signals
     *
     * This object needs to capture when it's systemd targets complete
     * so it can keep it's state updated
     *
     **/
    void subscribeToSystemdSignals();

    /** @brief Execute the transition request
     *
     * This function calls the appropriate systemd target for the input
     * transition.
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

    /** @brief Check if systemd state change is relevant to this object
     *
     * Instance specific interface to handle the detected systemd state
     * change
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    int sysStateChange(sdbusplus::message::message& msg);

    /** @brief Persistent sdbusplus DBus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief Used to subscribe to dbus systemd signals **/
    sdbusplus::bus::match_t systemdSignals;

    /** @brief Used to Set value of POHCounter */
    uint32_t pOHCounter(uint32_t value) override;

    /** @brief Used by the timer to update the POHCounter */
    void pOHCallback();

    /** @brief Used to restore POHCounter value from persisted file */
    void restorePOHCounter();

    /** @brief Serialize and persist requested POH counter.
     *
     *  @param[in] dir - pathname of file where the serialized POH counter will
     *                   be placed.
     *
     *  @return fs::path - pathname of persisted requested POH counter.
     */
    fs::path
        serializePOH(const fs::path& dir = fs::path(POH_COUNTER_PERSIST_PATH));

    /** @brief Deserialize a persisted requested POH counter.
     *
     *  @param[in] path - pathname of persisted POH counter file
     *  @param[in] retCounter - deserialized POH counter value
     *
     *  @return bool - true if the deserialization was successful, false
     *                 otherwise.
     */
    bool deserializePOH(const fs::path& path, uint32_t& retCounter);

    /** @brief Sets the LastStateChangeTime property and persists it. */
    void setStateChangeTime();

    /** @brief Serialize the last power state change time.
     *
     *  Save the time the state changed and the state itself.
     *  The state needs to be saved as well so that during rediscovery
     *  on reboots there's a way to know not to update the time again.
     */
    void serializeStateChangeTime();

    /** @brief Deserialize the last power state change time.
     *
     *  @param[out] time - Deserialized time
     *  @param[out] state - Deserialized power state
     *
     *  @return bool - true if successful, false otherwise.
     */
    bool deserializeStateChangeTime(uint64_t& time, PowerState& state);

    /** @brief Restores the power state change time.
     *
     *  The time is loaded into the LastStateChangeTime D-Bus property.
     *  On the very first start after this code has been applied but
     *  before the state has changed, the LastStateChangeTime value
     *  will be zero.
     */
    void restoreChassisStateChangeTime();

    /** @brief Timer used for tracking power on hours */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> pOHTimer;
};

} // namespace manager
} // namespace state
} // namespace phosphor
