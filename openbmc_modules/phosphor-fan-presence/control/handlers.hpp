#pragma once

namespace phosphor
{
namespace fan
{
namespace control
{
namespace handler
{

/**
 * @brief A handler function to set/update a property on a zone
 * @details Sets or updates a zone property to the given value using the
 * provided zone dbus object's set property function
 *
 * @param[in] intf - Interface on zone object
 * @param[in] prop - Property on interface
 * @param[in] func - Zone set property function pointer
 * @param[in] value - Value to set property to
 * @param[in] persist - Persist property value or not
 *
 * @return Lambda function
 *     A lambda function to set/update the zone property
 */
template <typename T>
auto setZoneProperty(const char* intf, const char* prop, T (Zone::*func)(T),
                     T&& value, bool persist)
{
    return [=, value = std::forward<T>(value)](auto& zone) {
        (zone.*func)(value);
        if (persist)
        {
            zone.setPersisted(intf, prop);
        }
    };
}

/**
 * @brief A handler function to set/update a property
 * @details Sets or updates a property's value determined by a combination of
 * an object's path, interface, and property names
 *
 * @param[in] path - Object's path name
 * @param[in] interface - Object's interface name
 * @param[in] property - Object's property name
 *
 * @return Lambda function
 *     A lambda function to set/update the property value
 */
template <typename T>
auto setProperty()
{
    return [](auto& zone, auto& path, auto& intf, auto& prop, T&& arg) {
        zone.setPropertyValue(path, intf, prop, std::forward<T>(arg));
    };
}

/**
 * @brief A handler function to set/update service name owner state
 * @details Sets or updates service name owner state used by a group where
 * a service name without an owner represents the service no longer exists
 *
 * @param[in] group - Group associated with a service
 *
 * @return Lambda function
 *     A lambda function to set/update the service name owner state
 */
auto setService(Group&& group)
{
    return [group = std::move(group)](auto& zone, auto& name, bool hasOwner) {
        // Update service name owner state list of a group
        zone.setServiceOwner(&group, name, hasOwner);
    };
}

/**
 * @brief A handler function to remove an interface from an object path
 * @details Removes an interface from an object's path which includes removing
 * all properties that would be under that interface
 *
 * @param[in] path - Object's path name
 * @param[in] interface - Object's interface name
 *
 * @return Lambda function
 *     A lambda function to remove the interface
 */
auto removeInterface(const char* path, const char* interface)
{
    return [=](auto& zone) { zone.removeObjectInterface(path, interface); };
}

} // namespace handler
} // namespace control
} // namespace fan
} // namespace phosphor
