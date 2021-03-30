#pragma once

#include "ledlayout.hpp"

#include <sdbusplus/bus.hpp>

#include <map>
#include <set>
#include <string>

namespace phosphor
{
namespace led
{

/** @brief Physical LED dbus constructs */
constexpr auto PHY_LED_PATH = "/xyz/openbmc_project/led/physical/";
constexpr auto PHY_LED_IFACE = "xyz.openbmc_project.Led.Physical";
constexpr auto DBUS_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";

/** @class Manager
 *  @brief Manages group of LEDs and applies action on the elements of group
 */
class Manager
{
  public:
    /** @brief Only need the default Manager */
    Manager() = delete;
    ~Manager() = default;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    /** @brief Special comparator for finding set difference */
    static bool ledComp(const phosphor::led::Layout::LedAction& left,
                        const phosphor::led::Layout::LedAction& right)
    {
        // Example :
        // If FIRST_1 is {fan0, 1, 1} and FIRST_2 is {fan0, 2, 2},
        // with default priority of Blink, this comparator would return
        // false. But considering the priority, this comparator would need
        // to return true so that we consider appropriate set and in
        // this case its {fan0, 1, 1}
        if (left.name == right.name)
        {
            if (left.action == right.action)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
        return left.name < right.name;
    }

    /** @brief Comparator for finding LEDs to be DeAsserted */
    static bool ledLess(const phosphor::led::Layout::LedAction& left,
                        const phosphor::led::Layout::LedAction& right)
    {
        return left.name < right.name;
    }

    /** @brief Comparator for helping unique_copy */
    static bool ledEqual(const phosphor::led::Layout::LedAction& left,
                         const phosphor::led::Layout::LedAction& right)
    {
        return left.name == right.name;
    }

    using group = std::set<phosphor::led::Layout::LedAction>;
    using LedLayout = std::map<std::string, group>;

    /** @brief static global map constructed at compile time */
    const LedLayout& ledMap;

    /** @brief Refer the user supplied LED layout and sdbusplus handler
     *
     *  @param [in] bus       - sdbusplus handler
     *  @param [in] LedLayout - LEDs group layout
     */
    Manager(sdbusplus::bus::bus& bus, const LedLayout& ledLayout) :
        ledMap(ledLayout), bus(bus)
    {
        // Nothing here
    }

    /** @brief Given a group name, applies the action on the group
     *
     *  @param[in]  path          -  dbus path of group
     *  @param[in]  assert        -  Could be true or false
     *  @param[in]  ledsAssert    -  LEDs that are to be asserted new
     *                               or to a different state
     *  @param[in]  ledsDeAssert  -  LEDs that are to be Deasserted
     *
     *  @return                   -  Success or exception thrown
     */
    bool setGroupState(const std::string& path, bool assert, group& ledsAssert,
                       group& ledsDeAssert);

    /** @brief Finds the set of LEDs to operate on and executes action
     *
     *  @param[in]  ledsAssert    -  LEDs that are to be asserted newly
     *                               or to a different state
     *  @param[in]  ledsDeAssert  -  LEDs that are to be Deasserted
     *
     *  @return: None
     */
    void driveLEDs(group& ledsAssert, group& ledsDeAssert);

  private:
    /** @brief sdbusplus handler */
    sdbusplus::bus::bus& bus;

    /** Map of physical LED path to service name */
    std::map<std::string, std::string> phyLeds{};

    /** @brief Pointers to groups that are in asserted state */
    std::set<const group*> assertedGroups;

    /** @brief Contains the highest priority actions for all
     *         asserted LEDs.
     */
    group currentState;

    /** @brief Contains the set of all actions for asserted LEDs */
    group combinedState;

    /** @brief Returns action string based on enum
     *
     *  @param[in]  action - Action enum
     *
     *  @return string equivalent of the passed in enumeration
     */
    static std::string getPhysicalAction(Layout::Action action);

    /** @brief Chooses appropriate action to be triggered on physical LED
     *  and calls into function that applies the actual action.
     *
     *  @param[in]  objPath   -  dbus object path
     *  @param[in]  action    -  Intended action to be triggered
     *  @param[in]  dutyOn    -  Duty Cycle ON percentage
     *  @param[in]  period    -  Time taken for one blink cycle
     */
    void drivePhysicalLED(const std::string& objPath, Layout::Action action,
                          uint8_t dutyOn, const uint16_t period);

    /** @brief Makes a dbus call to a passed in service name.
     *  This is now the physical LED controller
     *
     *  @param[in]  service   -  dbus service name
     *  @param[in]  objPath   -  dbus object path
     *  @param[in]  property  -  property to be written to
     *  @param[in]  value     -  Value to write
     */
    template <typename T>
    void drivePhysicalLED(const std::string& service,
                          const std::string& objPath,
                          const std::string& property, const T& value)
    {
        std::variant<T> data = value;

        auto method = bus.new_method_call(service.c_str(), objPath.c_str(),
                                          DBUS_PROPERTY_IFACE, "Set");
        method.append(PHY_LED_IFACE);
        method.append(property);
        method.append(data);

        // There will be exceptions going forward and hence don't need a
        // response
        bus.call_noreply(method);
        return;
    }

    /** @brief Populates map of Physical LED paths to service name */
    void populateObjectMap();
};

} // namespace led
} // namespace phosphor
