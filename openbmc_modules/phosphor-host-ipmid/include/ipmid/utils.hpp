#pragma once

#include <boost/system/error_code.hpp>
#include <chrono>
#include <ipmid/api-types.hpp>
#include <ipmid/message.hpp>
#include <ipmid/types.hpp>
#include <optional>
#include <sdbusplus/server.hpp>

namespace ipmi
{

using namespace std::literals::chrono_literals;

constexpr auto MAPPER_BUS_NAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_OBJ = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTF = "xyz.openbmc_project.ObjectMapper";

constexpr auto ROOT = "/";
constexpr auto HOST_MATCH = "host0";

constexpr auto PROP_INTF = "org.freedesktop.DBus.Properties";
constexpr auto DELETE_INTERFACE = "xyz.openbmc_project.Object.Delete";

constexpr auto METHOD_GET = "Get";
constexpr auto METHOD_GET_ALL = "GetAll";
constexpr auto METHOD_SET = "Set";

/* Use a value of 5s which aligns with BT/KCS bridged timeouts, rather
 * than the default 25s D-Bus timeout. */
constexpr std::chrono::microseconds IPMI_DBUS_TIMEOUT = 5s;

/** @class ServiceCache
 *  @brief Caches lookups of service names from the object mapper.
 *  @details Most ipmi commands need to talk to other dbus daemons to perform
 *           their intended actions on the BMC. This usually means they will
 *           first look up the service name providing the interface they
 *           require. This class reduces the number of such calls by caching
 *           the lookup for a specific service.
 */
class ServiceCache
{
  public:
    /** @brief Creates a new service cache for the given interface
     *         and path.
     *
     *  @param[in] intf - The interface used for each lookup
     *  @param[in] path - The path used for each lookup
     */
    ServiceCache(const std::string& intf, const std::string& path);
    ServiceCache(std::string&& intf, std::string&& path);

    /** @brief Gets the service name from the cache or does in a
     *         lookup when invalid.
     *
     *  @param[in] bus - The bus associated with and used for looking
     *                   up the service.
     */
    const std::string& getService(sdbusplus::bus::bus& bus);

    /** @brief Invalidates the current service name */
    void invalidate();

    /** @brief A wrapper around sdbusplus bus.new_method_call
     *
     *  @param[in] bus - The bus used for calling the method
     *  @param[in] intf - The interface containing the method
     *  @param[in] method - The method name
     *  @return The message containing the method call.
     */
    sdbusplus::message::message newMethodCall(sdbusplus::bus::bus& bus,
                                              const char* intf,
                                              const char* method);

    /** @brief Check to see if the current cache is valid
     *
     * @param[in] bus - The bus used for the service lookup
     * @return True if the cache is valid false otherwise.
     */
    bool isValid(sdbusplus::bus::bus& bus) const;

  private:
    /** @brief DBUS interface provided by the service */
    const std::string intf;
    /** @brief DBUS path provided by the service */
    const std::string path;
    /** @brief The name of the service if valid */
    std::optional<std::string> cachedService;
    /** @brief The name of the bus used in the service lookup */
    std::optional<std::string> cachedBusName;
};

/**
 * @brief Get the DBUS Service name for the input dbus path
 *
 * @param[in] bus - DBUS Bus Object
 * @param[in] intf - DBUS Interface
 * @param[in] path - DBUS Object Path
 *
 */
std::string getService(sdbusplus::bus::bus& bus, const std::string& intf,
                       const std::string& path);

/** @brief Gets the dbus object info implementing the given interface
 *         from the given subtree.
 *  @param[in] bus - DBUS Bus Object.
 *  @param[in] interface - Dbus interface.
 *  @param[in] subtreePath - subtree from where the search should start.
 *  @param[in] match - identifier for object.
 *  @return On success returns the object having objectpath and servicename.
 */
DbusObjectInfo getDbusObject(sdbusplus::bus::bus& bus,
                             const std::string& interface,
                             const std::string& subtreePath = ROOT,
                             const std::string& match = {});

/** @brief Gets the value associated with the given object
 *         and the interface.
 *  @param[in] bus - DBUS Bus Object.
 *  @param[in] service - Dbus service name.
 *  @param[in] objPath - Dbus object path.
 *  @param[in] interface - Dbus interface.
 *  @param[in] property - name of the property.
 *  @return On success returns the value of the property.
 */
Value getDbusProperty(sdbusplus::bus::bus& bus, const std::string& service,
                      const std::string& objPath, const std::string& interface,
                      const std::string& property,
                      std::chrono::microseconds timeout = IPMI_DBUS_TIMEOUT);

/** @brief Gets all the properties associated with the given object
 *         and the interface.
 *  @param[in] bus - DBUS Bus Object.
 *  @param[in] service - Dbus service name.
 *  @param[in] objPath - Dbus object path.
 *  @param[in] interface - Dbus interface.
 *  @return On success returns the map of name value pair.
 */
PropertyMap
    getAllDbusProperties(sdbusplus::bus::bus& bus, const std::string& service,
                         const std::string& objPath,
                         const std::string& interface,
                         std::chrono::microseconds timeout = IPMI_DBUS_TIMEOUT);

/** @brief Gets all managed objects associated with the given object
 *         path and service.
 *  @param[in] bus - D-Bus Bus Object.
 *  @param[in] service - D-Bus service name.
 *  @param[in] objPath - D-Bus object path.
 *  @return On success returns the map of name value pair.
 */
ObjectValueTree getManagedObjects(sdbusplus::bus::bus& bus,
                                  const std::string& service,
                                  const std::string& objPath);

/** @brief Sets the property value of the given object.
 *  @param[in] bus - DBUS Bus Object.
 *  @param[in] service - Dbus service name.
 *  @param[in] objPath - Dbus object path.
 *  @param[in] interface - Dbus interface.
 *  @param[in] property - name of the property.
 *  @param[in] value - value which needs to be set.
 */
void setDbusProperty(sdbusplus::bus::bus& bus, const std::string& service,
                     const std::string& objPath, const std::string& interface,
                     const std::string& property, const Value& value,
                     std::chrono::microseconds timeout = IPMI_DBUS_TIMEOUT);

/** @brief  Gets all the dbus objects from the given service root
 *          which matches the object identifier.
 *  @param[in] bus - DBUS Bus Object.
 *  @param[in] serviceRoot - Service root path.
 *  @param[in] interface - Dbus interface.
 *  @param[in] match - Identifier for a path.
 *  @returns map of object path and service info.
 */
ObjectTree getAllDbusObjects(sdbusplus::bus::bus& bus,
                             const std::string& serviceRoot,
                             const std::string& interface,
                             const std::string& match = {});

/** @brief Deletes all the dbus objects from the given service root
           which matches the object identifier.
 *  @param[in] bus - DBUS Bus Object.
 *  @param[in] serviceRoot - Service root path.
 *  @param[in] interface - Dbus interface.
 *  @param[in] match - Identifier for object.
 */
void deleteAllDbusObjects(sdbusplus::bus::bus& bus,
                          const std::string& serviceRoot,
                          const std::string& interface,
                          const std::string& match = {});

/** @brief Gets the ancestor objects of the given object
           which implements the given interface.
 *  @param[in] bus - Dbus bus object.
 *  @param[in] path - Child Dbus object path.
 *  @param[in] interfaces - Dbus interface list.
 *  @return map of object path and service info.
 */
ObjectTree getAllAncestors(sdbusplus::bus::bus& bus, const std::string& path,
                           InterfaceList&& interfaces);

/********* Begin co-routine yielding alternatives ***************/

/** @brief Get the D-Bus Service name for the input D-Bus path
 *
 *  @param[in] ctx - ipmi::Context::ptr
 *  @param[in] intf - D-Bus Interface
 *  @param[in] path - D-Bus Object Path
 *  @param[out] service - requested service name
 *  @return boost error code
 *
 */
boost::system::error_code getService(Context::ptr ctx, const std::string& intf,
                                     const std::string& path,
                                     std::string& service);

/** @brief Gets the D-Bus object info implementing the given interface
 *         from the given subtree.
 *  @param[in] ctx - ipmi::Context::ptr
 *  @param[in] interface - D-Bus interface.
 *  @param[in][optional] subtreePath - subtree from where the search starts.
 *  @param[in][optional] match - identifier for object.
 *  @param[out] D-Bus object with path and service name
 *  @return - boost error code object
 */
boost::system::error_code getDbusObject(Context::ptr ctx,
                                        const std::string& interface,
                                        const std::string& subtreePath,
                                        const std::string& match,
                                        DbusObjectInfo& dbusObject);

// default for ROOT for subtreePath and std::string{} for match
static inline boost::system::error_code
    getDbusObject(Context::ptr ctx, const std::string& interface,
                  DbusObjectInfo& dbusObject)
{
    return getDbusObject(ctx, interface, ROOT, {}, dbusObject);
}

// default std::string{} for match
static inline boost::system::error_code
    getDbusObject(Context::ptr ctx, const std::string& interface,
                  const std::string& subtreePath, DbusObjectInfo& dbusObject)
{
    return getDbusObject(ctx, interface, subtreePath, {}, dbusObject);
}

/** @brief Gets the value associated with the given object
 *         and the interface.
 *  @param[in] ctx - ipmi::Context::ptr
 *  @param[in] service - D-Bus service name.
 *  @param[in] objPath - D-Bus object path.
 *  @param[in] interface - D-Bus interface.
 *  @param[in] property - name of the property.
 *  @param[out] propertyValue - value of the D-Bus property.
 *  @return - boost error code object
 */
template <typename Type>
boost::system::error_code
    getDbusProperty(Context::ptr ctx, const std::string& service,
                    const std::string& objPath, const std::string& interface,
                    const std::string& property, Type& propertyValue)
{
    boost::system::error_code ec;
    auto variant = ctx->bus->yield_method_call<std::variant<Type>>(
        ctx->yield, ec, service.c_str(), objPath.c_str(), PROP_INTF, METHOD_GET,
        interface, property);
    if (!ec)
    {
        Type* tmp = std::get_if<Type>(&variant);
        if (tmp)
        {
            propertyValue = *tmp;
            return ec;
        }
        // user requested incorrect type; make an error code for them
        ec = boost::system::errc::make_error_code(
            boost::system::errc::invalid_argument);
    }
    return ec;
}

/** @brief Gets all the properties associated with the given object
 *         and the interface.
 *  @param[in] ctx - ipmi::Context::ptr
 *  @param[in] service - D-Bus service name.
 *  @param[in] objPath - D-Bus object path.
 *  @param[in] interface - D-Bus interface.
 *  @param[out] properties - map of name value pair.
 *  @return - boost error code object
 */
boost::system::error_code getAllDbusProperties(Context::ptr ctx,
                                               const std::string& service,
                                               const std::string& objPath,
                                               const std::string& interface,
                                               PropertyMap& properties);

/** @brief Sets the property value of the given object.
 *  @param[in] ctx - ipmi::Context::ptr
 *  @param[in] service - D-Bus service name.
 *  @param[in] objPath - D-Bus object path.
 *  @param[in] interface - D-Bus interface.
 *  @param[in] property - name of the property.
 *  @param[in] value - value which needs to be set.
 *  @return - boost error code object
 */
boost::system::error_code
    setDbusProperty(Context::ptr ctx, const std::string& service,
                    const std::string& objPath, const std::string& interface,
                    const std::string& property, const Value& value);

/** @brief  Gets all the D-Bus objects from the given service root
 *          which matches the object identifier.
 *  @param[in] ctx - ipmi::Context::ptr
 *  @param[in] serviceRoot - Service root path.
 *  @param[in] interface - D-Bus interface.
 *  @param[in][optional] match - Identifier for a path.
 *  @param[out] objectree - map of object path and service info.
 *  @return - boost error code object
 */
boost::system::error_code getAllDbusObjects(Context::ptr ctx,
                                            const std::string& serviceRoot,
                                            const std::string& interface,
                                            const std::string& match,
                                            ObjectTree& objectTree);

// default std::string{} for match
static inline boost::system::error_code
    getAllDbusObjects(Context::ptr ctx, const std::string& serviceRoot,
                      const std::string& interface, ObjectTree& objectTree)
{
    return getAllDbusObjects(ctx, serviceRoot, interface, {}, objectTree);
}

/** @brief Deletes all the D-Bus objects from the given service root
           which matches the object identifier.
 *  @param[in] ctx - ipmi::Context::ptr
 *  @param[out] ec - boost error code object
 *  @param[in] serviceRoot - Service root path.
 *  @param[in] interface - D-Bus interface.
 *  @param[in] match - Identifier for object.
 */
boost::system::error_code deleteAllDbusObjects(Context::ptr ctx,
                                               const std::string& serviceRoot,
                                               const std::string& interface,
                                               const std::string& match = {});

/** @brief Gets all managed objects associated with the given object
 *         path and service.
 *  @param[in] ctx - ipmi::Context::ptr
 *  @param[in] service - D-Bus service name.
 *  @param[in] objPath - D-Bus object path.
 *  @param[out] objects - map of name value pair.
 *  @return - boost error code object
 */
boost::system::error_code getManagedObjects(Context::ptr ctx,
                                            const std::string& service,
                                            const std::string& objPath,
                                            ObjectValueTree& objects);

/** @brief Gets the ancestor objects of the given object
           which implements the given interface.
 *  @param[in] ctx - ipmi::Context::ptr
 *  @param[in] path - Child D-Bus object path.
 *  @param[in] interfaces - D-Bus interface list.
 *  @param[out] ObjectTree - map of object path and service info.
 *  @return - boost error code object
 */
boost::system::error_code getAllAncestors(Context::ptr ctx,
                                          const std::string& path,
                                          const InterfaceList& interfaces,
                                          ObjectTree& objectTree);

/********* End co-routine yielding alternatives ***************/

/** @brief Retrieve the value from map of variants,
 *         returning a default if the key does not exist or the
 *         type of the value does not match the expected type
 *
 *  @tparam T - type of expected value to return
 *  @param[in] props - D-Bus propery map (Map of variants)
 *  @param[in] name - key name of property to fetch
 *  @param[in] defaultValue - default value to return on error
 *  @return - value from propery map at name, or defaultValue
 */
template <typename T>
T mappedVariant(const ipmi::PropertyMap& props, const std::string& name,
                const T& defaultValue)
{
    auto item = props.find(name);
    if (item == props.end())
    {
        return defaultValue;
    }
    const T* prop = std::get_if<T>(&item->second);
    if (!prop)
    {
        return defaultValue;
    }
    return *prop;
}

/** @struct VariantToDoubleVisitor
 *  @brief Visitor to convert variants to doubles
 *  @details Performs a static cast on the underlying type
 */
struct VariantToDoubleVisitor
{
    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        return static_cast<double>(t);
    }

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        throw std::invalid_argument("Cannot translate type to double");
    }
};

namespace method_no_args
{

/** @brief Calls the Dbus method which waits for response.
 *  @param[in] bus - DBUS Bus Object.
 *  @param[in] service - Dbus service name.
 *  @param[in] objPath - Dbus object path.
 *  @param[in] interface - Dbus interface.
 *  @param[in] method - Dbus method.
 */
void callDbusMethod(sdbusplus::bus::bus& bus, const std::string& service,
                    const std::string& objPath, const std::string& interface,
                    const std::string& method);

} // namespace method_no_args

/** @brief Perform the low-level i2c bus write-read.
 *  @param[in] i2cBus - i2c bus device node name, such as /dev/i2c-2.
 *  @param[in] slaveAddr - i2c device slave address.
 *  @param[in] writeData - The data written to i2c device.
 *  @param[out] readBuf - Data read from the i2c device.
 */
ipmi::Cc i2cWriteRead(std::string i2cBus, const uint8_t slaveAddr,
                      std::vector<uint8_t> writeData,
                      std::vector<uint8_t>& readBuf);
} // namespace ipmi
