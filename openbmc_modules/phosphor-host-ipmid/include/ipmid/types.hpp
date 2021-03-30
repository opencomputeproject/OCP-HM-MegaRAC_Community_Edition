#pragma once

#include <stdint.h>

#include <map>
#include <sdbusplus/server.hpp>
#include <string>
#include <variant>

namespace ipmi
{

using DbusObjectPath = std::string;
using DbusService = std::string;
using DbusInterface = std::string;
using DbusObjectInfo = std::pair<DbusObjectPath, DbusService>;
using DbusProperty = std::string;

using Value = std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                           int64_t, uint64_t, double, std::string>;

using PropertyMap = std::map<DbusProperty, Value>;

using ObjectTree =
    std::map<DbusObjectPath, std::map<DbusService, std::vector<DbusInterface>>>;

using InterfaceList = std::vector<std::string>;

using DbusInterfaceMap = std::map<DbusInterface, PropertyMap>;

using ObjectValueTree =
    std::map<sdbusplus::message::object_path, DbusInterfaceMap>;

namespace sensor
{

using Offset = uint8_t;

/**
 * @enum SkipAssertion
 * Matching value for skipping the update
 */
enum class SkipAssertion
{
    NONE,     // No skip defined
    ASSERT,   // Skip on Assert
    DEASSERT, // Skip on Deassert
};

struct Values
{
    SkipAssertion skip;
    Value assert;
    Value deassert;
};

/**
 * @enum PreReqValues
 * Pre-req conditions for a property.
 */
struct PreReqValues
{
    Value assert;   // Value in case of assert.
    Value deassert; // Value in case of deassert.
};

using PreReqOffsetValueMap = std::map<Offset, PreReqValues>;

/**
 * @struct SetSensorReadingReq
 *
 * IPMI Request data for Set Sensor Reading and Event Status Command
 */
struct SetSensorReadingReq
{
    uint8_t number;
    uint8_t operation;
    uint8_t reading;
    uint8_t assertOffset0_7;
    uint8_t assertOffset8_14;
    uint8_t deassertOffset0_7;
    uint8_t deassertOffset8_14;
    uint8_t eventData1;
    uint8_t eventData2;
    uint8_t eventData3;
} __attribute__((packed));

/**
 * @struct GetReadingResponse
 *
 * IPMI response data for Get Sensor Reading command.
 */
struct GetReadingResponse
{
    uint8_t reading;          //!< Sensor reading.
    uint8_t operation;        //!< Sensor scanning status / reading state.
    uint8_t assertOffset0_7;  //!< Discrete assertion states(0-7).
    uint8_t assertOffset8_14; //!< Discrete assertion states(8-14).
} __attribute__((packed));

constexpr auto inventoryRoot = "/xyz/openbmc_project/inventory";

struct GetSensorResponse
{
    uint8_t reading;                     // sensor reading
    bool readingOrStateUnavailable;      // 1 = reading/state unavailable
    bool scanningEnabled;                // 0 = sensor scanning disabled
    bool allEventMessagesEnabled;        // 0 = All Event Messages disabled
    uint8_t thresholdLevelsStates;       // threshold/discrete sensor states
    uint8_t discreteReadingSensorStates; // discrete-only, optional states
};

using OffsetValueMap = std::map<Offset, Values>;

using DbusPropertyValues = std::pair<PreReqOffsetValueMap, OffsetValueMap>;

using DbusPropertyMap = std::map<DbusProperty, DbusPropertyValues>;

using DbusInterfaceMap = std::map<DbusInterface, DbusPropertyMap>;

using InstancePath = std::string;
using Type = uint8_t;
using ReadingType = uint8_t;
using Multiplier = uint16_t;
using OffsetB = int16_t;
using Exponent = int8_t;
using ScaledOffset = double;
using Scale = int16_t;
using Unit = std::string;
using EntityType = uint8_t;
using EntityInst = uint8_t;
using SensorName = std::string;

enum class Mutability
{
    Read = 1 << 0,
    Write = 1 << 1,
};

inline Mutability operator|(Mutability lhs, Mutability rhs)
{
    return static_cast<Mutability>(static_cast<uint8_t>(lhs) |
                                   static_cast<uint8_t>(rhs));
}

inline Mutability operator&(Mutability lhs, Mutability rhs)
{
    return static_cast<Mutability>(static_cast<uint8_t>(lhs) &
                                   static_cast<uint8_t>(rhs));
}

struct Info
{
    EntityType entityType;
    EntityInst instance;
    Type sensorType;
    InstancePath sensorPath;
    DbusInterface sensorInterface;
    ReadingType sensorReadingType;
    Multiplier coefficientM;
    OffsetB coefficientB;
    Exponent exponentB;
    ScaledOffset scaledOffset;
    Exponent exponentR;
    bool hasScale;
    Scale scale;
    Unit unit;
    std::function<uint8_t(SetSensorReadingReq&, const Info&)> updateFunc;
    std::function<GetSensorResponse(const Info&)> getFunc;
    Mutability mutability;
    std::function<SensorName(const Info&)> sensorNameFunc;
    DbusInterfaceMap propertyInterfaces;
};

using Id = uint8_t;
using IdInfoMap = std::map<Id, Info>;

using PropertyMap = ipmi::PropertyMap;

using InterfaceMap = std::map<DbusInterface, PropertyMap>;

using Object = sdbusplus::message::object_path;
using ObjectMap = std::map<Object, InterfaceMap>;

using IpmiUpdateData = sdbusplus::message::message;

struct SelData
{
    Id sensorID;
    Type sensorType;
    ReadingType eventReadingType;
    Offset eventOffset;
};

using InventoryPath = std::string;

using InvObjectIDMap = std::map<InventoryPath, SelData>;

enum class ThresholdMask
{
    NON_CRITICAL_LOW_MASK = 0x01,
    CRITICAL_LOW_MASK = 0x02,
    NON_CRITICAL_HIGH_MASK = 0x08,
    CRITICAL_HIGH_MASK = 0x10,
};

static constexpr uint8_t maxContainedEntities = 4;
using ContainedEntitiesArray =
    std::array<std::pair<uint8_t, uint8_t>, maxContainedEntities>;

struct EntityInfo
{
    uint8_t containerEntityId;
    uint8_t containerEntityInstance;
    bool isList;
    bool isLinked;
    ContainedEntitiesArray containedEntities;
};

using EntityInfoMap = std::map<Id, EntityInfo>;

} // namespace sensor

namespace network
{
constexpr auto MAC_ADDRESS_FORMAT = "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx";

constexpr auto IPV4_ADDRESS_SIZE_BYTE = 4;
constexpr auto IPV6_ADDRESS_SIZE_BYTE = 16;

constexpr auto DEFAULT_MAC_ADDRESS = "00:00:00:00:00:00";
constexpr auto DEFAULT_ADDRESS = "0.0.0.0";

} // namespace network

} // namespace ipmi
