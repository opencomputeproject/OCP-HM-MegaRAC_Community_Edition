#include "manager.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Led/Physical/server.hpp>

#include <algorithm>
#include <iostream>
#include <string>
namespace phosphor
{
namespace led
{

// Assert -or- De-assert
bool Manager::setGroupState(const std::string& path, bool assert,
                            group& ledsAssert, group& ledsDeAssert)
{
    if (assert)
    {
        assertedGroups.insert(&ledMap.at(path));
    }
    else
    {
        auto search = assertedGroups.find(&ledMap.at(path));
        if (search != assertedGroups.end())
        {
            assertedGroups.erase(&ledMap.at(path));
        }
    }

    // This will contain the union of what's already in the asserted group
    group desiredState{};
    for (const auto& grp : assertedGroups)
    {
        desiredState.insert(grp->cbegin(), grp->cend());
    }

    // Find difference between Combined and Desired to identify
    // which LEDs are getting altered
    group transient{};
    std::set_difference(combinedState.begin(), combinedState.end(),
                        desiredState.begin(), desiredState.end(),
                        std::inserter(transient, transient.begin()), ledComp);
    if (transient.size())
    {
        // Find common LEDs between transient and Desired to know if some LEDs
        // are changing state and not really getting DeAsserted
        group ledsTransient{};
        std::set_intersection(
            transient.begin(), transient.end(), desiredState.begin(),
            desiredState.end(),
            std::inserter(ledsTransient, ledsTransient.begin()), ledLess);

        // Find difference between above 2 to identify those LEDs which are
        // really getting DeAsserted
        std::set_difference(transient.begin(), transient.end(),
                            ledsTransient.begin(), ledsTransient.end(),
                            std::inserter(ledsDeAssert, ledsDeAssert.begin()),
                            ledLess);

        // Remove the elements from Current that are being DeAsserted.
        if (ledsDeAssert.size())
        {
            // Power off LEDs that are to be really DeAsserted
            for (auto& it : ledsDeAssert)
            {
                // Update LEDs in "physically asserted" set by removing those
                // LEDs which are De-Asserted
                auto found = currentState.find(it);
                if (found != currentState.end())
                {
                    currentState.erase(found);
                }
            }
        }
    }

    // Now LEDs that are to be Asserted. These could either be fresh asserts
    // -or- change between [On]<-->[Blink]
    group temp{};
    std::unique_copy(desiredState.begin(), desiredState.end(),
                     std::inserter(temp, temp.begin()), ledEqual);
    if (temp.size())
    {
        // Find difference between [desired to be Asserted] and those LEDs
        // that are physically asserted currently.
        std::set_difference(
            temp.begin(), temp.end(), currentState.begin(), currentState.end(),
            std::inserter(ledsAssert, ledsAssert.begin()), ledComp);
    }

    // Update the current actual and desired(the virtual actual)
    currentState = std::move(temp);
    combinedState = std::move(desiredState);

    // If we survive, then set the state accordingly.
    return assert;
}

/** @brief Run through the map and apply action on the LEDs */
void Manager::driveLEDs(group& ledsAssert, group& ledsDeAssert)
{
    using namespace phosphor::logging;
    // Map of physical LED dbus paths to their Service providers
    populateObjectMap();

    if (phyLeds.empty())
    {
        // Error message is inside the map construction logic.
        return;
    }

    // This order of LED operation is important.
    if (ledsDeAssert.size())
    {
        for (const auto& it : ledsDeAssert)
        {
            std::string objPath = std::string(PHY_LED_PATH) + it.name;
            log<level::DEBUG>("De-Asserting LED",
                              entry("NAME=%s", it.name.c_str()));
            drivePhysicalLED(objPath, Layout::Action::Off, it.dutyOn,
                             it.period);
        }
    }

    if (ledsAssert.size())
    {
        for (const auto& it : ledsAssert)
        {
            std::string objPath = std::string(PHY_LED_PATH) + it.name;
            log<level::DEBUG>("Asserting LED",
                              entry("NAME=%s", it.name.c_str()));
            drivePhysicalLED(objPath, it.action, it.dutyOn, it.period);
        }
    }
    return;
}

// Calls into driving physical LED post choosing the action
void Manager::drivePhysicalLED(const std::string& objPath,
                               Layout::Action action, uint8_t dutyOn,
                               const uint16_t period)
{
    using namespace phosphor::logging;

    auto service = phyLeds.find(objPath);
    if (service == phyLeds.end() || service->second.empty())
    {
        log<level::ERR>("No service providers for physical LED",
                        entry("PATH=%s", objPath.c_str()));
        return;
    }

    // If Blink, set its property
    if (action == Layout::Action::Blink)
    {
        drivePhysicalLED(service->second, objPath, "DutyOn", dutyOn);
        drivePhysicalLED(service->second, objPath, "Period", period);
    }
    drivePhysicalLED(service->second, objPath, "State",
                     getPhysicalAction(action));
    return;
}

/** @brief Returns action string based on enum */
std::string Manager::getPhysicalAction(Layout::Action action)
{
    namespace server = sdbusplus::xyz::openbmc_project::Led::server;

    // TODO: openbmc/phosphor-led-manager#5
    //    Somehow need to use the generated Action enum than giving one
    //    in ledlayout.
    if (action == Layout::Action::On)
    {
        return server::convertForMessage(server::Physical::Action::On);
    }
    else if (action == Layout::Action::Blink)
    {
        return server::convertForMessage(server::Physical::Action::Blink);
    }
    else
    {
        return server::convertForMessage(server::Physical::Action::Off);
    }
}

/** Populates a map with physical LED paths to its service providers */
void Manager::populateObjectMap()
{
    using namespace phosphor::logging;

    // Mapper dbus constructs
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_OBJ_PATH = "/xyz/openbmc_project/object_mapper";
    constexpr auto MAPPER_IFACE = "xyz.openbmc_project.ObjectMapper";

    // Needed to be passed to get the SubTree level
    auto depth = 0;

    // Clean start
    phyLeds.clear();

    // Make a mapper call
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME, MAPPER_OBJ_PATH,
                                          MAPPER_IFACE, "GetSubTree");
    // Cook rest of the things.
    mapperCall.append(PHY_LED_PATH);
    mapperCall.append(depth);
    mapperCall.append(std::vector<std::string>({PHY_LED_IFACE}));

    auto reply = bus.call(mapperCall);
    if (reply.is_method_error())
    {
        // Its okay if we do not see a corresponding physical LED.
        log<level::INFO>("Error looking up Physical LED services",
                         entry("PATH=%s", PHY_LED_PATH));
        return;
    }

    // Response by mapper in the case of success
    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        objectTree;

    // This is the dict of object paths - service names - interfaces
    try
    {
        reply.read(objectTree);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to parse Physical LED service lookup",
                        entry("ERROR=%s", e.what()),
                        entry("REPLY_SIG=%s", reply.get_signature()));
        return;
    }
    if (objectTree.empty())
    {
        log<level::INFO>("Physical LED lookup did not return any services",
                         entry("PATH=%s", PHY_LED_PATH));
        return;
    }

    // Now construct our path -> Service name map.
    for (const auto& iter : objectTree)
    {
        phyLeds.emplace(iter.first, iter.second.begin()->first);
    }
    return;
}

} // namespace led
} // namespace phosphor
