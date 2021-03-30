#pragma once
#include "VariantVisitors.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/message/types.hpp>

#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

const constexpr char* jsonStore = "/var/configuration/flattened.json";
const constexpr char* inventoryPath = "/xyz/openbmc_project/inventory";
const constexpr char* entityManagerName = "xyz.openbmc_project.EntityManager";

constexpr const char* cpuInventoryPath =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard";
const std::regex illegalDbusRegex("[^A-Za-z0-9_]");

using BasicVariantType =
    std::variant<std::vector<std::string>, std::string, int64_t, uint64_t,
                 double, int32_t, uint32_t, int16_t, uint16_t, uint8_t, bool>;
using SensorBaseConfigMap =
    boost::container::flat_map<std::string, BasicVariantType>;
using SensorBaseConfiguration = std::pair<std::string, SensorBaseConfigMap>;
using SensorData = boost::container::flat_map<std::string, SensorBaseConfigMap>;
using ManagedObjectType =
    boost::container::flat_map<sdbusplus::message::object_path, SensorData>;

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;
using Association = std::tuple<std::string, std::string, std::string>;

bool findFiles(const std::filesystem::path dirPath,
               const std::string& matchString,
               std::vector<std::filesystem::path>& foundPaths,
               unsigned int symlinkDepth = 1);
bool isPowerOn(void);
bool hasBiosPost(void);
void setupPowerMatch(const std::shared_ptr<sdbusplus::asio::connection>& conn);
bool getSensorConfiguration(
    const std::string& type,
    const std::shared_ptr<sdbusplus::asio::connection>& dbusConnection,
    ManagedObjectType& resp, bool useCache = false);

void createAssociation(
    std::shared_ptr<sdbusplus::asio::dbus_interface>& association,
    const std::string& path);

// replaces limits if MinReading and MaxReading are found.
void findLimits(std::pair<double, double>& limits,
                const SensorBaseConfiguration* data);

enum class PowerState
{
    on,
    biosPost,
    always
};

namespace mapper
{
constexpr const char* busName = "xyz.openbmc_project.ObjectMapper";
constexpr const char* path = "/xyz/openbmc_project/object_mapper";
constexpr const char* interface = "xyz.openbmc_project.ObjectMapper";
constexpr const char* subtree = "GetSubTree";
} // namespace mapper

namespace properties
{
constexpr const char* interface = "org.freedesktop.DBus.Properties";
constexpr const char* get = "Get";
} // namespace properties

namespace power
{
const static constexpr char* busname = "xyz.openbmc_project.State.Host";
const static constexpr char* interface = "xyz.openbmc_project.State.Host";
const static constexpr char* path = "/xyz/openbmc_project/state/host0";
const static constexpr char* property = "CurrentHostState";
} // namespace power
namespace post
{
const static constexpr char* busname =
    "xyz.openbmc_project.State.OperatingSystem";
const static constexpr char* interface =
    "xyz.openbmc_project.State.OperatingSystem.Status";
const static constexpr char* path = "/xyz/openbmc_project/state/os";
const static constexpr char* property = "OperatingSystemState";
} // namespace post

namespace association
{
const static constexpr char* interface =
    "xyz.openbmc_project.Association.Definitions";
} // namespace association

template <typename T>
inline T loadVariant(
    const boost::container::flat_map<std::string, BasicVariantType>& data,
    const std::string& key)
{
    auto it = data.find(key);
    if (it == data.end())
    {
        std::cerr << "Configuration missing " << key << "\n";
        throw std::invalid_argument("Key Missing");
    }
    if constexpr (std::is_same_v<T, double>)
    {
        return std::visit(VariantToDoubleVisitor(), it->second);
    }
    else if constexpr (std::is_unsigned_v<T>)
    {
        return std::visit(VariantToUnsignedIntVisitor(), it->second);
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        return std::visit(VariantToStringVisitor(), it->second);
    }
    else
    {
        static_assert(!std::is_same_v<T, T>, "Type Not Implemented");
    }
}

inline void setReadState(const std::string& str, PowerState& val)
{

    if (str == "On")
    {
        val = PowerState::on;
    }
    else if (str == "BiosPost")
    {
        val = PowerState::biosPost;
    }
    else if (str == "Always")
    {
        val = PowerState::always;
    }
}

void createInventoryAssoc(
    std::shared_ptr<sdbusplus::asio::connection> conn,
    std::shared_ptr<sdbusplus::asio::dbus_interface> association,
    const std::string& path);

struct GetSensorConfiguration :
    std::enable_shared_from_this<GetSensorConfiguration>
{
    GetSensorConfiguration(
        std::shared_ptr<sdbusplus::asio::connection> connection,
        std::function<void(ManagedObjectType& resp)>&& callbackFunc) :
        dbusConnection(connection),
        callback(std::move(callbackFunc))
    {}
    void getConfiguration(const std::vector<std::string>& interfaces)
    {
        std::shared_ptr<GetSensorConfiguration> self = shared_from_this();
        dbusConnection->async_method_call(
            [self, interfaces](const boost::system::error_code ec,
                               const GetSubTreeType& ret) {
                if (ec)
                {
                    std::cerr << "Error calling mapper\n";
                    return;
                }
                for (const auto& [path, objDict] : ret)
                {
                    if (objDict.empty())
                    {
                        return;
                    }
                    const std::string& owner = objDict.begin()->first;

                    for (const std::string& interface : objDict.begin()->second)
                    {
                        // anything that starts with a requested configuration
                        // is good
                        if (std::find_if(
                                interfaces.begin(), interfaces.end(),
                                [interface](const std::string& possible) {
                                    return boost::starts_with(interface,
                                                              possible);
                                }) == interfaces.end())
                        {
                            continue;
                        }

                        self->dbusConnection->async_method_call(
                            [self, path, interface](
                                const boost::system::error_code ec,
                                boost::container::flat_map<
                                    std::string, BasicVariantType>& data) {
                                if (ec)
                                {
                                    std::cerr << "Error getting " << path
                                              << "\n";
                                    return;
                                }
                                self->respData[path][interface] =
                                    std::move(data);
                            },
                            owner, path, "org.freedesktop.DBus.Properties",
                            "GetAll", interface);
                    }
                }
            },
            mapper::busName, mapper::path, mapper::interface, mapper::subtree,
            "/", 0, interfaces);
    }

    ~GetSensorConfiguration()
    {
        callback(respData);
    }

    std::shared_ptr<sdbusplus::asio::connection> dbusConnection;
    std::function<void(ManagedObjectType& resp)> callback;
    ManagedObjectType respData;
};

// The common scheme for sysfs files naming is: <type><number>_<item>.
// This function returns optionally these 3 elements as a tuple.
std::optional<std::tuple<std::string, std::string, std::string>>
    splitFileName(const std::filesystem::path& filePath);
std::optional<double> readFile(const std::string& thresholdFile,
                               const double& scaleFactor);