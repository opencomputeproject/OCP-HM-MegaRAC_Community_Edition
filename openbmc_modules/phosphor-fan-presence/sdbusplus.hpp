#pragma once

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace fan
{
namespace util
{
namespace detail
{
namespace errors = sdbusplus::xyz::openbmc_project::Common::Error;
} // namespace detail

/**
 * @class DBusError
 *
 * The base class for the exceptions thrown on fails in the various
 * SDBusPlus calls.  Used so that a single catch statement can catch
 * any type of these exceptions.
 *
 * None of these exceptions will log anything when they are created,
 * it is up to the handler to do that if desired.
 */
class DBusError : public std::runtime_error
{
  public:
    explicit DBusError(const char* msg) : std::runtime_error(msg)
    {}
};

/**
 * @class DBusMethodError
 *
 * Thrown on a DBus Method call failure
 */
class DBusMethodError : public DBusError
{
  public:
    DBusMethodError(const std::string& busName, const std::string& path,
                    const std::string& interface, const std::string& method) :
        DBusError("DBus method call failed"),
        busName(busName), path(path), interface(interface), method(method)
    {}

    const std::string busName;
    const std::string path;
    const std::string interface;
    const std::string method;
};

/**
 * @class DBusServiceError
 *
 * Thrown when a service lookup fails.  Usually this points to
 * the object path not being present in D-Bus.
 */
class DBusServiceError : public DBusError
{
  public:
    DBusServiceError(const std::string& path, const std::string& interface) :
        DBusError("DBus service lookup failed"), path(path),
        interface(interface)
    {}

    const std::string path;
    const std::string interface;
};

/**
 * @class DBusPropertyError
 *
 * Thrown when a set/get property fails.
 */
class DBusPropertyError : public DBusError
{
  public:
    DBusPropertyError(const char* msg, const std::string& busName,
                      const std::string& path, const std::string& interface,
                      const std::string& property) :
        DBusError(msg),
        busName(busName), path(path), interface(interface), property(property)
    {}

    const std::string busName;
    const std::string path;
    const std::string interface;
    const std::string property;
};

/** @brief Alias for PropertiesChanged signal callbacks. */
template <typename... T>
using Properties = std::map<std::string, std::variant<T...>>;

/** @class SDBusPlus
 *  @brief DBus access delegate implementation for sdbusplus.
 */
class SDBusPlus
{

  public:
    /** @brief Get the bus connection. */
    static auto& getBus() __attribute__((pure))
    {
        static auto bus = sdbusplus::bus::new_default();
        return bus;
    }

    /** @brief Invoke a method. */
    template <typename... Args>
    static auto callMethod(sdbusplus::bus::bus& bus, const std::string& busName,
                           const std::string& path,
                           const std::string& interface,
                           const std::string& method, Args&&... args)
    {
        auto reqMsg = bus.new_method_call(busName.c_str(), path.c_str(),
                                          interface.c_str(), method.c_str());
        reqMsg.append(std::forward<Args>(args)...);
        try
        {
            auto respMsg = bus.call(reqMsg);
            if (respMsg.is_method_error())
            {
                throw DBusMethodError{busName, path, interface, method};
            }
            return respMsg;
        }
        catch (const sdbusplus::exception::SdBusError&)
        {
            throw DBusMethodError{busName, path, interface, method};
        }
    }

    /** @brief Invoke a method. */
    template <typename... Args>
    static auto callMethod(const std::string& busName, const std::string& path,
                           const std::string& interface,
                           const std::string& method, Args&&... args)
    {
        return callMethod(getBus(), busName, path, interface, method,
                          std::forward<Args>(args)...);
    }

    /** @brief Invoke a method and read the response. */
    template <typename Ret, typename... Args>
    static auto
        callMethodAndRead(sdbusplus::bus::bus& bus, const std::string& busName,
                          const std::string& path, const std::string& interface,
                          const std::string& method, Args&&... args)
    {
        sdbusplus::message::message respMsg = callMethod<Args...>(
            bus, busName, path, interface, method, std::forward<Args>(args)...);
        Ret resp;
        respMsg.read(resp);
        return resp;
    }

    /** @brief Invoke a method and read the response. */
    template <typename Ret, typename... Args>
    static auto callMethodAndRead(const std::string& busName,
                                  const std::string& path,
                                  const std::string& interface,
                                  const std::string& method, Args&&... args)
    {
        return callMethodAndRead<Ret>(getBus(), busName, path, interface,
                                      method, std::forward<Args>(args)...);
    }

    /** @brief Get subtree from the mapper without checking response. */
    static auto getSubTreeRaw(sdbusplus::bus::bus& bus, const std::string& path,
                              const std::string& interface, int32_t depth)
    {
        using namespace std::literals::string_literals;

        using Path = std::string;
        using Intf = std::string;
        using Serv = std::string;
        using Intfs = std::vector<Intf>;
        using Objects = std::map<Path, std::map<Serv, Intfs>>;
        Intfs intfs = {interface};

        return callMethodAndRead<Objects>(bus,
                                          "xyz.openbmc_project.ObjectMapper"s,
                                          "/xyz/openbmc_project/object_mapper"s,
                                          "xyz.openbmc_project.ObjectMapper"s,
                                          "GetSubTree"s, path, depth, intfs);
    }

    /** @brief Get subtree from the mapper. */
    static auto getSubTree(sdbusplus::bus::bus& bus, const std::string& path,
                           const std::string& interface, int32_t depth)
    {
        auto mapperResp = getSubTreeRaw(bus, path, interface, depth);
        if (mapperResp.empty())
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Empty response from mapper GetSubTree",
                phosphor::logging::entry("SUBTREE=%s", path.c_str()),
                phosphor::logging::entry("INTERFACE=%s", interface.c_str()),
                phosphor::logging::entry("DEPTH=%u", depth));
            phosphor::logging::elog<detail::errors::InternalFailure>();
        }
        return mapperResp;
    }

    /** @brief Get service from the mapper. */
    static auto getService(sdbusplus::bus::bus& bus, const std::string& path,
                           const std::string& interface)
    {
        using namespace std::literals::string_literals;
        using GetObject = std::map<std::string, std::vector<std::string>>;

        try
        {
            auto mapperResp = callMethodAndRead<GetObject>(
                bus, "xyz.openbmc_project.ObjectMapper"s,
                "/xyz/openbmc_project/object_mapper"s,
                "xyz.openbmc_project.ObjectMapper"s, "GetObject"s, path,
                GetObject::mapped_type{interface});

            if (mapperResp.empty())
            {
                // Should never happen.  A missing object would fail
                // in callMethodAndRead()
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Empty mapper response on service lookup");
                throw DBusServiceError{path, interface};
            }
            return mapperResp.begin()->first;
        }
        catch (DBusMethodError& e)
        {
            throw DBusServiceError{path, interface};
        }
    }

    /** @brief Get service from the mapper. */
    static auto getService(const std::string& path,
                           const std::string& interface)
    {
        return getService(getBus(), path, interface);
    }

    /** @brief Get a property with mapper lookup. */
    template <typename Property>
    static auto getProperty(sdbusplus::bus::bus& bus, const std::string& path,
                            const std::string& interface,
                            const std::string& property)
    {
        using namespace std::literals::string_literals;

        auto service = getService(bus, path, interface);
        auto msg =
            callMethod(bus, service, path, "org.freedesktop.DBus.Properties"s,
                       "Get"s, interface, property);
        if (msg.is_method_error())
        {
            throw DBusPropertyError{"DBus get property failed", service, path,
                                    interface, property};
        }
        std::variant<Property> value;
        msg.read(value);
        return std::get<Property>(value);
    }

    /** @brief Get a property with mapper lookup. */
    template <typename Property>
    static auto getProperty(const std::string& path,
                            const std::string& interface,
                            const std::string& property)
    {
        return getProperty<Property>(getBus(), path, interface, property);
    }

    /** @brief Get a property variant with mapper lookup. */
    template <typename Variant>
    static auto getPropertyVariant(sdbusplus::bus::bus& bus,
                                   const std::string& path,
                                   const std::string& interface,
                                   const std::string& property)
    {
        using namespace std::literals::string_literals;

        auto service = getService(bus, path, interface);
        auto msg =
            callMethod(bus, service, path, "org.freedesktop.DBus.Properties"s,
                       "Get"s, interface, property);
        if (msg.is_method_error())
        {
            throw DBusPropertyError{"DBus get property variant failed", service,
                                    path, interface, property};
        }
        Variant value;
        msg.read(value);
        return value;
    }

    /** @brief Get a property variant with mapper lookup. */
    template <typename Variant>
    static auto getPropertyVariant(const std::string& path,
                                   const std::string& interface,
                                   const std::string& property)
    {
        return getPropertyVariant<Variant>(getBus(), path, interface, property);
    }

    /** @brief Get a property without mapper lookup. */
    template <typename Property>
    static auto getProperty(sdbusplus::bus::bus& bus,
                            const std::string& service, const std::string& path,
                            const std::string& interface,
                            const std::string& property)
    {
        using namespace std::literals::string_literals;

        auto msg = callMethodAndReturn(bus, service, path,
                                       "org.freedesktop.DBus.Properties"s,
                                       "Get"s, interface, property);
        if (msg.is_method_error())
        {
            throw DBusPropertyError{"DBus get property failed", service, path,
                                    interface, property};
        }
        std::variant<Property> value;
        msg.read(value);
        return std::get<Property>(value);
    }

    /** @brief Get a property without mapper lookup. */
    template <typename Property>
    static auto getProperty(const std::string& service, const std::string& path,
                            const std::string& interface,
                            const std::string& property)
    {
        return getProperty<Property>(getBus(), service, path, interface,
                                     property);
    }

    /** @brief Get a property variant without mapper lookup. */
    template <typename Variant>
    static auto getPropertyVariant(sdbusplus::bus::bus& bus,
                                   const std::string& service,
                                   const std::string& path,
                                   const std::string& interface,
                                   const std::string& property)
    {
        using namespace std::literals::string_literals;

        auto msg = callMethodAndReturn(bus, service, path,
                                       "org.freedesktop.DBus.Properties"s,
                                       "Get"s, interface, property);
        if (msg.is_method_error())
        {
            throw DBusPropertyError{"DBus get property variant failed", service,
                                    path, interface, property};
        }
        Variant value;
        msg.read(value);
        return value;
    }

    /** @brief Get a property variant without mapper lookup. */
    template <typename Variant>
    static auto getPropertyVariant(const std::string& service,
                                   const std::string& path,
                                   const std::string& interface,
                                   const std::string& property)
    {
        return getPropertyVariant<Variant>(getBus(), service, path, interface,
                                           property);
    }

    /** @brief Set a property with mapper lookup. */
    template <typename Property>
    static void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                            const std::string& interface,
                            const std::string& property, Property&& value)
    {
        using namespace std::literals::string_literals;

        std::variant<Property> varValue(std::forward<Property>(value));

        auto service = getService(bus, path, interface);
        auto msg = callMethodAndReturn(bus, service, path,
                                       "org.freedesktop.DBus.Properties"s,
                                       "Set"s, interface, property, varValue);
        if (msg.is_method_error())
        {
            throw DBusPropertyError{"DBus set property failed", service, path,
                                    interface, property};
        }
    }

    /** @brief Set a property with mapper lookup. */
    template <typename Property>
    static void setProperty(const std::string& path,
                            const std::string& interface,
                            const std::string& property, Property&& value)
    {
        return setProperty(getBus(), path, interface, property,
                           std::forward<Property>(value));
    }

    /** @brief Set a property without mapper lookup. */
    template <typename Property>
    static void setProperty(sdbusplus::bus::bus& bus,
                            const std::string& service, const std::string& path,
                            const std::string& interface,
                            const std::string& property, Property&& value)
    {
        using namespace std::literals::string_literals;

        std::variant<Property> varValue(std::forward<Property>(value));

        auto msg = callMethodAndReturn(bus, service, path,
                                       "org.freedesktop.DBus.Properties"s,
                                       "Set"s, interface, property, varValue);
        if (msg.is_method_error())
        {
            throw DBusPropertyError{"DBus set property failed", service, path,
                                    interface, property};
        }
    }

    /** @brief Set a property without mapper lookup. */
    template <typename Property>
    static void setProperty(const std::string& service, const std::string& path,
                            const std::string& interface,
                            const std::string& property, Property&& value)
    {
        return setProperty(getBus(), service, path, interface, property,
                           std::forward<Property>(value));
    }

    /** @brief Invoke method with mapper lookup. */
    template <typename... Args>
    static auto lookupAndCallMethod(sdbusplus::bus::bus& bus,
                                    const std::string& path,
                                    const std::string& interface,
                                    const std::string& method, Args&&... args)
    {
        return callMethod(bus, getService(bus, path, interface), path,
                          interface, method, std::forward<Args>(args)...);
    }

    /** @brief Invoke method with mapper lookup. */
    template <typename... Args>
    static auto lookupAndCallMethod(const std::string& path,
                                    const std::string& interface,
                                    const std::string& method, Args&&... args)
    {
        return lookupAndCallMethod(getBus(), path, interface, method,
                                   std::forward<Args>(args)...);
    }

    /** @brief Invoke method and read with mapper lookup. */
    template <typename Ret, typename... Args>
    static auto lookupCallMethodAndRead(sdbusplus::bus::bus& bus,
                                        const std::string& path,
                                        const std::string& interface,
                                        const std::string& method,
                                        Args&&... args)
    {
        return callMethodAndRead(bus, getService(bus, path, interface), path,
                                 interface, method,
                                 std::forward<Args>(args)...);
    }

    /** @brief Invoke method and read with mapper lookup. */
    template <typename Ret, typename... Args>
    static auto lookupCallMethodAndRead(const std::string& path,
                                        const std::string& interface,
                                        const std::string& method,
                                        Args&&... args)
    {
        return lookupCallMethodAndRead<Ret>(getBus(), path, interface, method,
                                            std::forward<Args>(args)...);
    }

    /** @brief Invoke a method and return without checking for error. */
    template <typename... Args>
    static auto callMethodAndReturn(sdbusplus::bus::bus& bus,
                                    const std::string& busName,
                                    const std::string& path,
                                    const std::string& interface,
                                    const std::string& method, Args&&... args)
    {
        auto reqMsg = bus.new_method_call(busName.c_str(), path.c_str(),
                                          interface.c_str(), method.c_str());
        reqMsg.append(std::forward<Args>(args)...);
        auto respMsg = bus.call(reqMsg);

        return respMsg;
    }
};

} // namespace util
} // namespace fan
} // namespace phosphor
