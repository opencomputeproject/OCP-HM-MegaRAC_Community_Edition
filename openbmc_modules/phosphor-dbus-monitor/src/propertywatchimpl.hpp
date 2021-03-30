#pragma once

#include "callback.hpp"
#include "data_types.hpp"
#include "propertywatch.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <string>
#include <vector>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

using MappedPropertyIndex =
    RefKeyMap<const std::string,
              RefKeyMap<const std::string, RefVector<const std::string>>>;

MappedPropertyIndex convert(const PropertyIndex& index);

template <typename DBusInterfaceType>
void PropertyWatch<DBusInterfaceType>::start()
{
    if (alreadyRan)
    {
        return;
    }

    // The index has a flat layout which is not optimal here.  Nest
    // properties in a map of interface names in a map of object paths.
    auto mapped = convert(index);

    for (const auto& m : mapped)
    {
        const auto& path = m.first.get();
        const auto& interfaces = m.second;

        // Watch for new interfaces on this path.
        DBusInterfaceType::addMatch(
            sdbusplus::bus::match::rules::interfacesAdded(path),
            [this](auto& msg)
            // *INDENT-OFF*
            { this->interfacesAdded(msg); });
        // *INDENT-ON*

        // Do a query to populate the cache.  Start with a mapper query.
        // The specific services are queried below.
        auto getObjectFromMapper = [](const auto& path) {
            const std::vector<std::string> queryInterfaces; // all interfaces
            try
            {
                return DBusInterfaceType::template callMethodAndRead<GetObject>(
                    MAPPER_BUSNAME, MAPPER_PATH, MAPPER_INTERFACE, "GetObject",
                    path, queryInterfaces);
            }
            catch (const sdbusplus::exception::SdBusError&)
            {
                // Paths in the configuration may not exist yet.  Prime those
                // later, when/if InterfacesAdded occurs.
                return GetObject();
            }
        };
        auto mapperResp = getObjectFromMapper(path);

        for (const auto& i : interfaces)
        {
            const auto& interface = i.first.get();

            // Watch for property changes on this interface.
            DBusInterfaceType::addMatch(
                sdbusplus::bus::match::rules::propertiesChanged(path,
                                                                interface),
                [this](auto& msg)
                // *INDENT-OFF*
                {
                    std::string interface;
                    msg.read(interface);
                    auto path = msg.get_path();
                    this->propertiesChanged(msg, path, interface);
                });
            // *INDENT-ON*

            // The mapper response is a busname:[interfaces] map.  Look for
            // each interface in the index and if found, query the service and
            // populate the cache entries for the interface.
            for (const auto& mr : mapperResp)
            {
                const auto& busName = mr.first;
                const auto& mapperInterfaces = mr.second;
                if (mapperInterfaces.end() ==
                    std::find(mapperInterfaces.begin(), mapperInterfaces.end(),
                              interface))
                {
                    // This interface isn't being watched.
                    continue;
                }

                // Delegate type specific property updates to subclasses.
                try
                {
                    updateProperties(busName, path, interface);
                }
                catch (const sdbusplus::exception::SdBusError&)
                {
                    // If for some reason the path has gone away since
                    // the mapper lookup we'll simply try again if/when
                    // InterfacesAdded occurs the next time it shows up.
                }
            }
        }
    }

    alreadyRan = true;
}

template <typename DBusInterfaceType>
void PropertyWatch<DBusInterfaceType>::callback(Context ctx)
{
    // Invoke callback if present.
    if (this->alreadyRan && this->cb)
    {
        (*this->cb)(ctx);
    }
}

template <typename T, typename DBusInterfaceType>
void PropertyWatchOfType<T, DBusInterfaceType>::updateProperties(
    const std::string& busName, const std::string& path,
    const std::string& interface)
{
    auto properties =
        DBusInterfaceType::template callMethodAndRead<PropertiesChanged<T>>(
            busName.c_str(), path.c_str(), "org.freedesktop.DBus.Properties",
            "GetAll", interface);
    propertiesChanged(path, interface, properties);
}

template <typename T, typename DBusInterfaceType>
void PropertyWatchOfType<T, DBusInterfaceType>::propertiesChanged(
    const std::string& path, const std::string& interface,
    const PropertiesChanged<T>& properties)
{
    // Update the cache for any watched properties.
    for (const auto& p : properties)
    {
        auto key = std::make_tuple(path, interface, p.first);
        auto item = this->index.find(key);
        if (item == this->index.end())
        {
            // This property isn't being watched.
            continue;
        }

        // Run property value thru filter operations
        auto isFiltered = false;
        const auto& storage = std::get<storageIndex>(item->second);
        auto value = std::get<T>(p.second);
        if (filterOps)
        {
            any_ns::any anyValue = value;
            if ((*filterOps)(anyValue))
            {
                // Property value filtered, clear it from storage so
                // callback functions do not use it
                isFiltered = true;
                std::get<valueIndex>(storage.get()).clear();
            }
        }
        if (!isFiltered)
        {
            // Property value not filtered out, update
            std::get<valueIndex>(storage.get()) = value;
            // Invoke callback if present.
            this->callback(Context::SIGNAL);
        }
    }
}

template <typename T, typename DBusInterfaceType>
void PropertyWatchOfType<T, DBusInterfaceType>::propertiesChanged(
    sdbusplus::message::message& msg, const std::string& path,
    const std::string& interface)
{
    PropertiesChanged<T> properties;
    msg.read(properties);
    propertiesChanged(path, interface, properties);
}

template <typename T, typename DBusInterfaceType>
void PropertyWatchOfType<T, DBusInterfaceType>::interfacesAdded(
    const std::string& path, const InterfacesAdded<T>& interfaces)
{
    for (const auto& i : interfaces)
    {
        propertiesChanged(path, i.first, i.second);
    }
}

template <typename T, typename DBusInterfaceType>
void PropertyWatchOfType<T, DBusInterfaceType>::interfacesAdded(
    sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path path;
    InterfacesAdded<T> interfaces;
    msg.read(path, interfaces);
    interfacesAdded(path, interfaces);
}

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
