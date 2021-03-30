#pragma once

#include "dbus_types.hpp"

#include <sdbusplus/bus/match.hpp>

namespace openpower::pels
{

using sdbusplus::exception::SdBusError;
namespace match_rules = sdbusplus::bus::match::rules;

/**
 * @class DBusWatcher
 *
 * The base class for the PropertyWatcher and InterfaceWatcher classes.
 */
class DBusWatcher
{
  public:
    DBusWatcher() = delete;
    virtual ~DBusWatcher() = default;
    DBusWatcher(const DBusWatcher&) = default;
    DBusWatcher& operator=(const DBusWatcher&) = default;
    DBusWatcher(DBusWatcher&&) = default;
    DBusWatcher& operator=(DBusWatcher&&) = default;

    /**
     * @brief Constructor
     *
     * @param[in] path - The D-Bus path that will be watched
     * @param[in] interface - The D-Bus interface that will be watched
     */
    DBusWatcher(const std::string& path, const std::string& interface) :
        _path(path), _interface(interface)
    {
    }

  protected:
    /**
     * @brief The D-Bus path
     */
    std::string _path;

    /**
     * @brief The D-Bus interface
     */
    std::string _interface;

    /**
     * @brief The match objects for the propertiesChanged and
     *        interfacesAdded signals.
     */
    std::vector<sdbusplus::bus::match_t> _matches;
};

/**
 * @class PropertyWatcher
 *
 * This class allows the user to be kept up to data with a D-Bus
 * property's value.  It does this by calling a user specified function
 * that is passed the variant that contains the property's value when:
 *
 * 1) The property is read when the class is constructed, if
 *    the property is on D-Bus at the time.
 * 2) The property changes (via a property changed signal).
 * 3) An interfacesAdded signal is received with that property.
 *
 * The DataInterface class is used to access D-Bus, and is a template
 * to avoid any circular include issues as that class is one of the
 * users of this one.
 */
template <typename DataIface>
class PropertyWatcher : public DBusWatcher
{
  public:
    PropertyWatcher() = delete;
    ~PropertyWatcher() = default;
    PropertyWatcher(const PropertyWatcher&) = delete;
    PropertyWatcher& operator=(const PropertyWatcher&) = delete;
    PropertyWatcher(PropertyWatcher&&) = delete;
    PropertyWatcher& operator=(PropertyWatcher&&) = delete;

    using PropertySetFunc = std::function<void(const DBusValue&)>;

    /**
     * @brief Constructor
     *
     * Reads the property if it is on D-Bus, and sets up the match
     * objects for the propertiesChanged and interfacesAdded signals.
     *
     * @param[in] bus - The sdbusplus bus object
     * @param[in] path - The D-Bus path of the property
     * @param[in] interface - The D-Bus interface that contains the property
     * @param[in] propertyName - The property name
     * @param[in] service - The D-Bus service to use for the property read.
     *                      Can be empty to look it up instead.
     * @param[in] dataIface - The DataInterface object
     * @param[in] func - The callback used any time the property is read
     */
    PropertyWatcher(sdbusplus::bus::bus& bus, const std::string& path,
                    const std::string& interface,
                    const std::string& propertyName, const std::string& service,
                    const DataIface& dataIface, PropertySetFunc func) :
        DBusWatcher(path, interface),
        _name(propertyName), _setFunc(func)
    {
        _matches.emplace_back(
            bus, match_rules::propertiesChanged(_path, _interface),
            std::bind(std::mem_fn(&PropertyWatcher::propChanged), this,
                      std::placeholders::_1));

        _matches.emplace_back(
            bus,
            match_rules::interfacesAdded() + match_rules::argNpath(0, _path),
            std::bind(std::mem_fn(&PropertyWatcher::interfaceAdded), this,
                      std::placeholders::_1));

        try
        {
            read(dataIface, service);
        }
        catch (const SdBusError& e)
        {
            // Path doesn't exist now
        }
    }

    /**
     * @brief Constructor
     *
     * Reads the property if it is on D-Bus, and sets up the match
     * objects for the propertiesChanged and interfacesAdded signals.
     *
     * Unlike the other constructor, this contructor doesn't take the
     * service to use for the property read so it will look it up with
     * an ObjectMapper GetObject call.
     *
     * @param[in] bus - The sdbusplus bus object
     * @param[in] path - The D-Bus path of the property
     * @param[in] interface - The D-Bus interface that contains the property
     * @param[in] propertyName - The property name
     * @param[in] dataIface - The DataInterface object
     * @param[in] func - The callback used any time the property is read
     */
    PropertyWatcher(sdbusplus::bus::bus& bus, const std::string& path,
                    const std::string& interface,
                    const std::string& propertyName, const DataIface& dataIface,
                    PropertySetFunc func) :
        PropertyWatcher(bus, path, interface, propertyName, "", dataIface, func)
    {
    }

    /**
     * @brief Reads the property on D-Bus, and calls
     *        the user defined function with the value.
     *
     * If the passed in service is empty, look up the service to use.
     *
     * @param[in] dataIface - The DataInterface object
     * @param[in] service - The D-Bus service to make the getProperty
     *                      call with, if not empty
     */
    void read(const DataIface& dataIface, std::string service)
    {
        if (service.empty())
        {
            service = dataIface.getService(_path, _interface);
        }

        if (!service.empty())
        {
            DBusValue value;
            dataIface.getProperty(service, _path, _interface, _name, value);

            _setFunc(value);
        }
    }

    /**
     * @brief The propertiesChanged callback
     *
     * Calls the user defined function with the property value
     *
     * @param[in] msg - The sdbusplus message object
     */
    void propChanged(sdbusplus::message::message& msg)
    {
        DBusInterface interface;
        DBusPropertyMap properties;

        msg.read(interface, properties);

        auto prop = properties.find(_name);
        if (prop != properties.end())
        {
            _setFunc(prop->second);
        }
    }

    /**
     * @brief The interfacesAdded callback
     *
     * Calls the user defined function with the property value
     *
     * @param[in] msg - The sdbusplus message object
     */
    void interfaceAdded(sdbusplus::message::message& msg)
    {
        sdbusplus::message::object_path path;
        DBusInterfaceMap interfaces;

        msg.read(path, interfaces);

        auto iface = interfaces.find(_interface);
        if (iface != interfaces.end())
        {
            auto prop = iface->second.find(_name);
            if (prop != iface->second.end())
            {
                _setFunc(prop->second);
            }
        }
    }

  private:
    /**
     * @brief The D-Bus property name
     */
    std::string _name;

    /**
     * @brief The function that will be called any time the
     *        property is read.
     */
    PropertySetFunc _setFunc;
};

/**
 * @class InterfaceWatcher
 *
 * This class allows the user to be kept up to data with a D-Bus
 * interface's properties..  It does this by calling a user specified
 * function that is passed a map of the D-Bus property names and values
 * on that interface when:
 *
 * 1) The interface is read when the class is constructed, if
 *    the interface is on D-Bus at the time.
 * 2) The interface has a property that changes (via a property changed signal).
 * 3) An interfacesAdded signal is received.
 *
 * The DataInterface class is used to access D-Bus, and is a template
 * to avoid any circular include issues as that class is one of the
 * users of this one.
 */
template <typename DataIface>
class InterfaceWatcher : public DBusWatcher
{
  public:
    InterfaceWatcher() = delete;
    ~InterfaceWatcher() = default;
    InterfaceWatcher(const InterfaceWatcher&) = delete;
    InterfaceWatcher& operator=(const InterfaceWatcher&) = delete;
    InterfaceWatcher(InterfaceWatcher&&) = delete;
    InterfaceWatcher& operator=(InterfaceWatcher&&) = delete;

    using InterfaceSetFunc = std::function<void(const DBusPropertyMap&)>;

    /**
     * @brief Constructor
     *
     * Reads all properties on the interface if it is on D-Bus,
     * and sets up the match objects for the propertiesChanged
     * and interfacesAdded signals.
     *
     * @param[in] bus - The sdbusplus bus object
     * @param[in] path - The D-Bus path of the property
     * @param[in] interface - The D-Bus interface that contains the property
     * @param[in] dataIface - The DataInterface object
     * @param[in] func - The callback used any time the property is read
     */
    InterfaceWatcher(sdbusplus::bus::bus& bus, const std::string& path,
                     const std::string& interface, const DataIface& dataIface,
                     InterfaceSetFunc func) :
        DBusWatcher(path, interface),
        _setFunc(func)
    {
        _matches.emplace_back(
            bus, match_rules::propertiesChanged(_path, _interface),
            std::bind(std::mem_fn(&InterfaceWatcher::propChanged), this,
                      std::placeholders::_1));

        _matches.emplace_back(
            bus,
            match_rules::interfacesAdded() + match_rules::argNpath(0, _path),
            std::bind(std::mem_fn(&InterfaceWatcher::interfaceAdded), this,
                      std::placeholders::_1));

        try
        {
            read(dataIface);
        }
        catch (const SdBusError& e)
        {
            // Path doesn't exist now
        }
    }

    /**
     * @brief Reads the interface's properties on D-Bus, and
     * calls the the user defined function with the property map.
     *
     * @param[in] dataIface - The DataInterface object
     */
    void read(const DataIface& dataIface)
    {
        auto service = dataIface.getService(_path, _interface);
        if (!service.empty())
        {
            auto properties =
                dataIface.getAllProperties(service, _path, _interface);

            _setFunc(properties);
        }
    }

    /**
     * @brief The propertiesChanged callback
     *
     * Calls the user defined function with the property map.  Only the
     * properties that changed will be in the map.
     *
     * @param[in] msg - The sdbusplus message object
     */
    void propChanged(sdbusplus::message::message& msg)
    {
        DBusInterface interface;
        DBusPropertyMap properties;

        msg.read(interface, properties);

        _setFunc(properties);
    }

    /**
     * @brief The interfacesAdded callback
     *
     * Calls the user defined function with the property map
     *
     * @param[in] msg - The sdbusplus message object
     */
    void interfaceAdded(sdbusplus::message::message& msg)
    {
        sdbusplus::message::object_path path;
        DBusInterfaceMap interfaces;

        msg.read(path, interfaces);

        auto iface = interfaces.find(_interface);
        if (iface != interfaces.end())
        {
            _setFunc(iface->second);
        }
    }

  private:
    /**
     * @brief The function that will be called any time the
     *        interface is read.
     */
    InterfaceSetFunc _setFunc;
};

} // namespace openpower::pels
