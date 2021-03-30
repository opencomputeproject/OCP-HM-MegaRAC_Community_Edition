#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

using namespace phosphor::logging;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

namespace phosphor
{
namespace fan
{
namespace util
{

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

constexpr auto OPERATIONAL_STATUS_INTF =
    "xyz.openbmc_project.State.Decorator.OperationalStatus";
constexpr auto FUNCTIONAL_PROPERTY = "Functional";

class FileDescriptor
{
  public:
    FileDescriptor() = delete;
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor(FileDescriptor&&) = default;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    FileDescriptor& operator=(FileDescriptor&&) = default;

    explicit FileDescriptor(int fd) : fd(fd)
    {}

    ~FileDescriptor()
    {
        if (fd != -1)
        {
            close(fd);
        }
    }

    int operator()()
    {
        return fd;
    }

    void open(const std::string& pathname, int flags)
    {
        fd = ::open(pathname.c_str(), flags);
        if (-1 == fd)
        {
            log<level::ERR>("Failed to open file device: ",
                            entry("PATHNAME=%s", pathname.c_str()));
            elog<InternalFailure>();
        }
    }

    bool is_open()
    {
        return fd != -1;
    }

  private:
    int fd = -1;
};

/**
 * @brief Get the object map for creating or updating an object property
 *
 * @param[in] path - The dbus object path name
 * @param[in] intf - The dbus interface
 * @param[in] prop - The dbus property
 * @param[in] value - The property value
 *
 * @return - The full object path containing the property value
 */
template <typename T>
auto getObjMap(const std::string& path, const std::string& intf,
               const std::string& prop, const T& value)
{
    using Property = std::string;
    using Value = std::variant<T>;
    using PropertyMap = std::map<Property, Value>;

    using Interface = std::string;
    using InterfaceMap = std::map<Interface, PropertyMap>;

    using Object = sdbusplus::message::object_path;
    using ObjectMap = std::map<Object, InterfaceMap>;

    ObjectMap objectMap;
    InterfaceMap interfaceMap;
    PropertyMap propertyMap;

    propertyMap.emplace(prop, value);
    interfaceMap.emplace(intf, std::move(propertyMap));
    objectMap.emplace(path, std::move(interfaceMap));

    return objectMap;
}

} // namespace util
} // namespace fan
} // namespace phosphor
