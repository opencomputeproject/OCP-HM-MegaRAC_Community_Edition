#pragma once

#include "sysfs.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Led/Physical/server.hpp>

#include <fstream>
#include <string>

namespace phosphor
{
namespace led
{
/** @brief De-assert value */
constexpr unsigned long DEASSERT = 0;

/** @class Physical
 *  @brief Responsible for applying actions on a particular physical LED
 */
class Physical :
    public sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::Led::server::Physical>
{
  public:
    Physical() = delete;
    ~Physical() = default;
    Physical(const Physical&) = delete;
    Physical& operator=(const Physical&) = delete;
    Physical(Physical&&) = delete;
    Physical& operator=(Physical&&) = delete;

    /** @brief Constructs LED object. Argument 'true' says that we hold off
     *   from sending the signals since we need to do some house keeping and
     *   only when we finish that, we are considered active and can then
     *   broadcast the signal.
     *
     * @param[in] bus       - system dbus handler
     * @param[in] objPath   - The Dbus path that hosts physical LED
     * @param[in] ledPath   - sysfs path where this LED is exported
     * @param[in] color     - led color name
     */
    Physical(sdbusplus::bus::bus& bus, const std::string& objPath,
             SysfsLed& led, const std::string& color = "") :

        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Led::server::Physical>(
            bus, objPath.c_str(), true),
        led(led)
    {
        // Suppose this is getting launched as part of BMC reboot, then we
        // need to save what the micro-controller currently has.
        setInitialState();

        // Read led color from enviroment and set it in DBus.
        setLedColor(color);

        // We are now ready.
        emit_object_added();
    }

    /** @brief Overloaded State Property Setter function
     *
     *  @param[in] value   -  One of OFF / ON / BLINK
     *  @return            -  Success or exception thrown
     */
    Action state(Action value) override;

  private:
    /** @brief Associated LED implementation
     */
    SysfsLed& led;

    /** @brief The value that will assert the LED */
    unsigned long assert;

    /** @brief reads sysfs and then setsup the parameteres accordingly
     *
     *  @return None
     */
    void setInitialState();

    /** @brief Applies the user triggered action on the LED
     *   by writing to sysfs
     *
     *  @param [in] current - Current state of LED
     *  @param [in] request - Requested state
     *
     *  @return None
     */
    void driveLED(Action current, Action request);

    /** @brief Sets the LED to either ON or OFF state
     *
     *  @param [in] action - Requested action. Could be OFF or ON
     *  @return None
     */
    void stableStateOperation(Action action);

    /** @brief Sets the LED to BLINKING
     *
     *  @return None
     */
    void blinkOperation();

    /** @brief set led color property in DBus
     *
     *  @param[in] color - led color name
     */
    void setLedColor(const std::string& color);
};

} // namespace led
} // namespace phosphor
