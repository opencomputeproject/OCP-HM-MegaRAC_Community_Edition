#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <ipmid/utils.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/message/types.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace ipmi
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

namespace network
{

/** @brief checks if the given ip is Link Local Ip or not.
 *  @param[in] ipaddress - IPAddress.
 */
bool isLinkLocalIP(const std::string& ipaddress);

} // namespace network

// TODO There may be cases where an interface is implemented by multiple
//  objects,to handle such cases we are interested on that object
//  which are on interested busname.
//  Currently mapper doesn't give the readable busname(gives busid) so we can't
//  use busname to find the object,will do later once the support is there.

DbusObjectInfo getDbusObject(sdbusplus::bus::bus& bus,
                             const std::string& interface,
                             const std::string& serviceRoot,
                             const std::string& match)
{
    std::vector<DbusInterface> interfaces;
    interfaces.emplace_back(interface);

    auto depth = 0;

    auto mapperCall = bus.new_method_call(MAPPER_BUS_NAME, MAPPER_OBJ,
                                          MAPPER_INTF, "GetSubTree");

    mapperCall.append(serviceRoot, depth, interfaces);

    auto mapperReply = bus.call(mapperCall);
    if (mapperReply.is_method_error())
    {
        log<level::ERR>("Error in mapper call");
        elog<InternalFailure>();
    }

    ObjectTree objectTree;
    mapperReply.read(objectTree);

    if (objectTree.empty())
    {
        log<level::ERR>("No Object has implemented the interface",
                        entry("INTERFACE=%s", interface.c_str()));
        elog<InternalFailure>();
    }

    DbusObjectInfo objectInfo;

    // if match is empty then return the first object
    if (match == "")
    {
        objectInfo = std::make_pair(
            objectTree.begin()->first,
            std::move(objectTree.begin()->second.begin()->first));
        return objectInfo;
    }

    // else search the match string in the object path
    auto found = std::find_if(
        objectTree.begin(), objectTree.end(), [&match](const auto& object) {
            return (object.first.find(match) != std::string::npos);
        });

    if (found == objectTree.end())
    {
        log<level::ERR>("Failed to find object which matches",
                        entry("MATCH=%s", match.c_str()));
        elog<InternalFailure>();
        // elog<> throws an exception.
    }

    return make_pair(found->first, std::move(found->second.begin()->first));
}

Value getDbusProperty(sdbusplus::bus::bus& bus, const std::string& service,
                      const std::string& objPath, const std::string& interface,
                      const std::string& property,
                      std::chrono::microseconds timeout)
{

    Value value;

    auto method = bus.new_method_call(service.c_str(), objPath.c_str(),
                                      PROP_INTF, METHOD_GET);

    method.append(interface, property);

    auto reply = bus.call(method, timeout.count());

    if (reply.is_method_error())
    {
        log<level::ERR>("Failed to get property",
                        entry("PROPERTY=%s", property.c_str()),
                        entry("PATH=%s", objPath.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
        elog<InternalFailure>();
    }

    reply.read(value);

    return value;
}

PropertyMap getAllDbusProperties(sdbusplus::bus::bus& bus,
                                 const std::string& service,
                                 const std::string& objPath,
                                 const std::string& interface,
                                 std::chrono::microseconds timeout)
{
    PropertyMap properties;

    auto method = bus.new_method_call(service.c_str(), objPath.c_str(),
                                      PROP_INTF, METHOD_GET_ALL);

    method.append(interface);

    auto reply = bus.call(method, timeout.count());

    if (reply.is_method_error())
    {
        log<level::ERR>("Failed to get all properties",
                        entry("PATH=%s", objPath.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
        elog<InternalFailure>();
    }

    reply.read(properties);
    return properties;
}

ObjectValueTree getManagedObjects(sdbusplus::bus::bus& bus,
                                  const std::string& service,
                                  const std::string& objPath)
{
    ipmi::ObjectValueTree interfaces;

    auto method = bus.new_method_call(service.c_str(), objPath.c_str(),
                                      "org.freedesktop.DBus.ObjectManager",
                                      "GetManagedObjects");

    auto reply = bus.call(method);

    if (reply.is_method_error())
    {
        log<level::ERR>("Failed to get managed objects",
                        entry("PATH=%s", objPath.c_str()));
        elog<InternalFailure>();
    }

    reply.read(interfaces);
    return interfaces;
}

void setDbusProperty(sdbusplus::bus::bus& bus, const std::string& service,
                     const std::string& objPath, const std::string& interface,
                     const std::string& property, const Value& value,
                     std::chrono::microseconds timeout)
{
    auto method = bus.new_method_call(service.c_str(), objPath.c_str(),
                                      PROP_INTF, METHOD_SET);

    method.append(interface, property, value);

    if (!bus.call(method, timeout.count()))
    {
        log<level::ERR>("Failed to set property",
                        entry("PROPERTY=%s", property.c_str()),
                        entry("PATH=%s", objPath.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
        elog<InternalFailure>();
    }
}

ServiceCache::ServiceCache(const std::string& intf, const std::string& path) :
    intf(intf), path(path), cachedService(std::nullopt),
    cachedBusName(std::nullopt)
{
}

ServiceCache::ServiceCache(std::string&& intf, std::string&& path) :
    intf(std::move(intf)), path(std::move(path)), cachedService(std::nullopt),
    cachedBusName(std::nullopt)
{
}

const std::string& ServiceCache::getService(sdbusplus::bus::bus& bus)
{
    if (!isValid(bus))
    {
        cachedBusName = bus.get_unique_name();
        cachedService = ::ipmi::getService(bus, intf, path);
    }
    return cachedService.value();
}

void ServiceCache::invalidate()
{
    cachedBusName = std::nullopt;
    cachedService = std::nullopt;
}

sdbusplus::message::message
    ServiceCache::newMethodCall(sdbusplus::bus::bus& bus, const char* intf,
                                const char* method)
{
    return bus.new_method_call(getService(bus).c_str(), path.c_str(), intf,
                               method);
}

bool ServiceCache::isValid(sdbusplus::bus::bus& bus) const
{
    return cachedService && cachedBusName == bus.get_unique_name();
}

std::string getService(sdbusplus::bus::bus& bus, const std::string& intf,
                       const std::string& path)
{
    auto mapperCall =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetObject");

    mapperCall.append(path);
    mapperCall.append(std::vector<std::string>({intf}));

    auto mapperResponseMsg = bus.call(mapperCall);

    if (mapperResponseMsg.is_method_error())
    {
        throw std::runtime_error("ERROR in mapper call");
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);

    if (mapperResponse.begin() == mapperResponse.end())
    {
        throw std::runtime_error("ERROR in reading the mapper response");
    }

    return mapperResponse.begin()->first;
}

ipmi::ObjectTree getAllDbusObjects(sdbusplus::bus::bus& bus,
                                   const std::string& serviceRoot,
                                   const std::string& interface,
                                   const std::string& match)
{
    std::vector<std::string> interfaces;
    interfaces.emplace_back(interface);

    auto depth = 0;

    auto mapperCall = bus.new_method_call(MAPPER_BUS_NAME, MAPPER_OBJ,
                                          MAPPER_INTF, "GetSubTree");

    mapperCall.append(serviceRoot, depth, interfaces);

    auto mapperReply = bus.call(mapperCall);
    if (mapperReply.is_method_error())
    {
        log<level::ERR>("Error in mapper call",
                        entry("SERVICEROOT=%s", serviceRoot.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));

        elog<InternalFailure>();
    }

    ObjectTree objectTree;
    mapperReply.read(objectTree);

    for (auto it = objectTree.begin(); it != objectTree.end();)
    {
        if (it->first.find(match) == std::string::npos)
        {
            it = objectTree.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return objectTree;
}

void deleteAllDbusObjects(sdbusplus::bus::bus& bus,
                          const std::string& serviceRoot,
                          const std::string& interface,
                          const std::string& match)
{
    try
    {
        auto objectTree = getAllDbusObjects(bus, serviceRoot, interface, match);

        for (auto& object : objectTree)
        {
            method_no_args::callDbusMethod(bus, object.second.begin()->first,
                                           object.first, DELETE_INTERFACE,
                                           "Delete");
        }
    }
    catch (sdbusplus::exception::exception& e)
    {
        log<level::INFO>("sdbusplus exception - Unable to delete the objects",
                         entry("ERROR=%s", e.what()),
                         entry("INTERFACE=%s", interface.c_str()),
                         entry("SERVICE=%s", serviceRoot.c_str()));
    }
}

static inline std::string convertToString(const InterfaceList& interfaces)
{
    std::string intfStr;
    for (const auto& intf : interfaces)
    {
        intfStr += "," + intf;
    }
    return intfStr;
}

ObjectTree getAllAncestors(sdbusplus::bus::bus& bus, const std::string& path,
                           InterfaceList&& interfaces)
{
    auto mapperCall = bus.new_method_call(MAPPER_BUS_NAME, MAPPER_OBJ,
                                          MAPPER_INTF, "GetAncestors");
    mapperCall.append(path, interfaces);

    auto mapperReply = bus.call(mapperCall);
    if (mapperReply.is_method_error())
    {
        log<level::ERR>(
            "Error in mapper call", entry("PATH=%s", path.c_str()),
            entry("INTERFACES=%s", convertToString(interfaces).c_str()));

        elog<InternalFailure>();
    }

    ObjectTree objectTree;
    mapperReply.read(objectTree);

    if (objectTree.empty())
    {
        log<level::ERR>(
            "No Object has implemented the interface",
            entry("PATH=%s", path.c_str()),
            entry("INTERFACES=%s", convertToString(interfaces).c_str()));
        elog<InternalFailure>();
    }

    return objectTree;
}

namespace method_no_args
{

void callDbusMethod(sdbusplus::bus::bus& bus, const std::string& service,
                    const std::string& objPath, const std::string& interface,
                    const std::string& method)

{
    auto busMethod = bus.new_method_call(service.c_str(), objPath.c_str(),
                                         interface.c_str(), method.c_str());

    auto reply = bus.call(busMethod);

    if (reply.is_method_error())
    {
        log<level::ERR>("Failed to execute method",
                        entry("METHOD=%s", method.c_str()),
                        entry("PATH=%s", objPath.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
        elog<InternalFailure>();
    }
}

} // namespace method_no_args

/********* Begin co-routine yielding alternatives ***************/

boost::system::error_code getService(Context::ptr ctx, const std::string& intf,
                                     const std::string& path,
                                     std::string& service)
{
    boost::system::error_code ec;
    std::map<std::string, std::vector<std::string>> mapperResponse =
        ctx->bus->yield_method_call<decltype(mapperResponse)>(
            ctx->yield, ec, "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject", path,
            std::vector<std::string>({intf}));

    if (!ec)
    {
        service = std::move(mapperResponse.begin()->first);
    }
    return ec;
}

boost::system::error_code getDbusObject(Context::ptr ctx,
                                        const std::string& interface,
                                        const std::string& subtreePath,
                                        const std::string& match,
                                        DbusObjectInfo& dbusObject)
{
    std::vector<DbusInterface> interfaces;
    interfaces.emplace_back(interface);

    auto depth = 0;
    boost::system::error_code ec;
    ObjectTree objectTree = ctx->bus->yield_method_call<ObjectTree>(
        ctx->yield, ec, MAPPER_BUS_NAME, MAPPER_OBJ, MAPPER_INTF, "GetSubTree",
        subtreePath, depth, interfaces);

    if (ec)
    {
        return ec;
    }

    if (objectTree.empty())
    {
        log<level::ERR>("No Object has implemented the interface",
                        entry("INTERFACE=%s", interface.c_str()),
                        entry("NETFN=%x", ctx->netFn),
                        entry("CMD=%x,", ctx->cmd));
        return boost::system::errc::make_error_code(
            boost::system::errc::no_such_process);
    }

    // if match is empty then return the first object
    if (match == "")
    {
        dbusObject = std::make_pair(
            std::move(objectTree.begin()->first),
            std::move(objectTree.begin()->second.begin()->first));
        return ec;
    }

    // else search the match string in the object path
    auto found = std::find_if(
        objectTree.begin(), objectTree.end(), [&match](const auto& object) {
            return (object.first.find(match) != std::string::npos);
        });

    if (found == objectTree.end())
    {
        log<level::ERR>("Failed to find object which matches",
                        entry("MATCH=%s", match.c_str()),
                        entry("NETFN=%x", ctx->netFn),
                        entry("CMD=%x,", ctx->cmd));
        // set ec
        return boost::system::errc::make_error_code(
            boost::system::errc::no_such_file_or_directory);
    }

    dbusObject = std::make_pair(std::move(found->first),
                                std::move(found->second.begin()->first));
    return ec;
}

boost::system::error_code getAllDbusProperties(Context::ptr ctx,
                                               const std::string& service,
                                               const std::string& objPath,
                                               const std::string& interface,
                                               PropertyMap& properties)
{
    boost::system::error_code ec;
    properties = ctx->bus->yield_method_call<PropertyMap>(
        ctx->yield, ec, service.c_str(), objPath.c_str(), PROP_INTF,
        METHOD_GET_ALL, interface);
    return ec;
}

boost::system::error_code
    setDbusProperty(Context::ptr ctx, const std::string& service,
                    const std::string& objPath, const std::string& interface,
                    const std::string& property, const Value& value)
{
    boost::system::error_code ec;
    ctx->bus->yield_method_call(ctx->yield, ec, service.c_str(),
                                objPath.c_str(), PROP_INTF, METHOD_SET,
                                interface, property, value);
    return ec;
}

boost::system::error_code getAllDbusObjects(Context::ptr ctx,
                                            const std::string& serviceRoot,
                                            const std::string& interface,
                                            const std::string& match,
                                            ObjectTree& objectTree)
{
    boost::system::error_code ec;
    std::vector<std::string> interfaces;
    interfaces.emplace_back(interface);

    auto depth = 0;

    objectTree = ctx->bus->yield_method_call<ObjectTree>(
        ctx->yield, ec, MAPPER_BUS_NAME, MAPPER_OBJ, MAPPER_INTF, "GetSubTree",
        serviceRoot, depth, interfaces);

    if (ec)
    {
        return ec;
    }

    for (auto it = objectTree.begin(); it != objectTree.end();)
    {
        if (it->first.find(match) == std::string::npos)
        {
            it = objectTree.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return ec;
}

boost::system::error_code deleteAllDbusObjects(Context::ptr ctx,
                                               const std::string& serviceRoot,
                                               const std::string& interface,
                                               const std::string& match)
{
    ObjectTree objectTree;
    boost::system::error_code ec =
        getAllDbusObjects(ctx, serviceRoot, interface, match, objectTree);
    if (ec)
    {
        return ec;
    }

    for (auto& object : objectTree)
    {
        ctx->bus->yield_method_call(ctx->yield, ec,
                                    object.second.begin()->first, object.first,
                                    DELETE_INTERFACE, "Delete");
        if (ec)
        {
            log<level::ERR>("Failed to delete all objects",
                            entry("INTERFACE=%s", interface.c_str()),
                            entry("SERVICE=%s", serviceRoot.c_str()),
                            entry("NETFN=%x", ctx->netFn),
                            entry("CMD=%x,", ctx->cmd),
                            entry("ERROR=%s", ec.message().c_str()));
            break;
        }
    }
    return ec;
}

boost::system::error_code getManagedObjects(Context::ptr ctx,
                                            const std::string& service,
                                            const std::string& objPath,
                                            ObjectValueTree& objects)
{
    boost::system::error_code ec;
    objects = ctx->bus->yield_method_call<ipmi::ObjectValueTree>(
        ctx->yield, ec, service.c_str(), objPath.c_str(),
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    return ec;
}

boost::system::error_code getAllAncestors(Context::ptr ctx,
                                          const std::string& path,
                                          const InterfaceList& interfaces,
                                          ObjectTree& objectTree)
{
    std::string interfaceList = convertToString(interfaces);

    boost::system::error_code ec;
    objectTree = ctx->bus->yield_method_call<ObjectTree>(
        ctx->yield, ec, MAPPER_BUS_NAME, MAPPER_OBJ, MAPPER_INTF,
        "GetAncestors", path, interfaceList);

    if (ec)
    {
        return ec;
    }

    if (objectTree.empty())
    {
        log<level::ERR>("No Object has implemented the interface",
                        entry("PATH=%s", path.c_str()),
                        entry("INTERFACES=%s", interfaceList.c_str()));
        elog<InternalFailure>();
    }

    return ec;
}

/********* End co-routine yielding alternatives ***************/

ipmi::Cc i2cWriteRead(std::string i2cBus, const uint8_t slaveAddr,
                      std::vector<uint8_t> writeData,
                      std::vector<uint8_t>& readBuf)
{
    // Open the i2c device, for low-level combined data write/read
    int i2cDev = ::open(i2cBus.c_str(), O_RDWR | O_CLOEXEC);
    if (i2cDev < 0)
    {
        log<level::ERR>("Failed to open i2c bus",
                        phosphor::logging::entry("BUS=%s", i2cBus.c_str()));
        return ipmi::ccInvalidFieldRequest;
    }

    const size_t writeCount = writeData.size();
    const size_t readCount = readBuf.size();
    int msgCount = 0;
    i2c_msg i2cmsg[2] = {0};
    if (writeCount)
    {
        // Data will be writtern to the slave address
        i2cmsg[msgCount].addr = slaveAddr;
        i2cmsg[msgCount].flags = 0x00;
        i2cmsg[msgCount].len = writeCount;
        i2cmsg[msgCount].buf = writeData.data();
        msgCount++;
    }
    if (readCount)
    {
        // Data will be read into the buffer from the slave address
        i2cmsg[msgCount].addr = slaveAddr;
        i2cmsg[msgCount].flags = I2C_M_RD;
        i2cmsg[msgCount].len = readCount;
        i2cmsg[msgCount].buf = readBuf.data();
        msgCount++;
    }

    i2c_rdwr_ioctl_data msgReadWrite = {0};
    msgReadWrite.msgs = i2cmsg;
    msgReadWrite.nmsgs = msgCount;

    // Perform the combined write/read
    int ret = ::ioctl(i2cDev, I2C_RDWR, &msgReadWrite);
    ::close(i2cDev);

    if (ret < 0)
    {
        log<level::ERR>("I2C WR Failed!",
                        phosphor::logging::entry("RET=%d", ret));
        return ipmi::ccUnspecifiedError;
    }
    if (readCount)
    {
        readBuf.resize(msgReadWrite.msgs[msgCount - 1].len);
    }

    return ipmi::ccSuccess;
}

} // namespace ipmi
