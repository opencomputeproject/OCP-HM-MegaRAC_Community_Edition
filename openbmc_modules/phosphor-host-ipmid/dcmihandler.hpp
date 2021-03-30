#pragma once

#include "nlohmann/json.hpp"

#include <map>
#include <sdbusplus/bus.hpp>
#include <string>
#include <vector>

namespace dcmi
{

using NumInstances = size_t;
using Json = nlohmann::json;

enum Commands
{
    // Get capability bits
    GET_CAPABILITIES = 0x01,
    GET_POWER_READING = 0x02,
    GET_POWER_LIMIT = 0x03,
    SET_POWER_LIMIT = 0x04,
    APPLY_POWER_LIMIT = 0x05,
    GET_ASSET_TAG = 0x06,
    GET_SENSOR_INFO = 0x07,
    SET_ASSET_TAG = 0x08,
    GET_MGMNT_CTRL_ID_STR = 0x09,
    SET_MGMNT_CTRL_ID_STR = 0x0A,
    GET_TEMP_READINGS = 0x10,
    SET_CONF_PARAMS = 0x12,
    GET_CONF_PARAMS = 0x13,
};

static constexpr auto propIntf = "org.freedesktop.DBus.Properties";
static constexpr auto assetTagIntf =
    "xyz.openbmc_project.Inventory.Decorator.AssetTag";
static constexpr auto assetTagProp = "AssetTag";
static constexpr auto networkServiceName = "xyz.openbmc_project.Network";
static constexpr auto networkConfigObj = "/xyz/openbmc_project/network/config";
static constexpr auto networkConfigIntf =
    "xyz.openbmc_project.Network.SystemConfiguration";
static constexpr auto hostNameProp = "HostName";
static constexpr auto temperatureSensorType = 0x01;
static constexpr auto maxInstances = 255;
static constexpr auto gDCMISensorsConfig =
    "/usr/share/ipmi-providers/dcmi_sensors.json";
static constexpr auto ethernetIntf =
    "xyz.openbmc_project.Network.EthernetInterface";
static constexpr auto ethernetDefaultChannelNum = 0x1;
static constexpr auto networkRoot = "/xyz/openbmc_project/network";
static constexpr auto dhcpObj = "/xyz/openbmc_project/network/config/dhcp";
static constexpr auto dhcpIntf =
    "xyz.openbmc_project.Network.DHCPConfiguration";
static constexpr auto systemBusName = "org.freedesktop.systemd1";
static constexpr auto systemPath = "/org/freedesktop/systemd1";
static constexpr auto systemIntf = "org.freedesktop.systemd1.Manager";
static constexpr auto gDCMICapabilitiesConfig =
    "/usr/share/ipmi-providers/dcmi_cap.json";
static constexpr auto gDCMIPowerMgmtCapability = "PowerManagement";
static constexpr auto gDCMIPowerMgmtSupported = 0x1;
static constexpr auto gMaxSELEntriesMask = 0xFFF;
static constexpr auto gByteBitSize = 8;

namespace assettag
{

using ObjectPath = std::string;
using Service = std::string;
using Interfaces = std::vector<std::string>;
using ObjectTree = std::map<ObjectPath, std::map<Service, Interfaces>>;

} // namespace assettag

namespace temp_readings
{
static constexpr auto maxDataSets = 8;
static constexpr auto maxTemp = 127; // degrees C

/** @struct Response
 *
 *  DCMI payload for Get Temperature Readings response
 */
struct Response
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t temperature : 7; //!< Temperature reading in Celsius
    uint8_t sign : 1;        //!< Sign bit
#endif
#if BYTE_ORDER == BIG_ENDIAN
    uint8_t sign : 1;        //!< Sign bit
    uint8_t temperature : 7; //!< Temperature reading in Celsius
#endif
    uint8_t instance; //!< Entity instance number
} __attribute__((packed));

using ResponseList = std::vector<Response>;
using Value = uint8_t;
using Sign = bool;
using Temperature = std::tuple<Value, Sign>;
} // namespace temp_readings

namespace sensor_info
{
static constexpr auto maxRecords = 8;

/** @struct Response
 *
 *  DCMI payload for Get Sensor Info response
 */
struct Response
{
    uint8_t recordIdLsb; //!< SDR record id LS byte
    uint8_t recordIdMsb; //!< SDR record id MS byte
} __attribute__((packed));

using ResponseList = std::vector<Response>;
} // namespace sensor_info

static constexpr auto groupExtId = 0xDC;

static constexpr auto assetTagMaxOffset = 62;
static constexpr auto assetTagMaxSize = 63;
static constexpr auto maxBytes = 16;
static constexpr size_t maxCtrlIdStrLen = 63;

/** @struct GetAssetTagRequest
 *
 *  DCMI payload for Get Asset Tag command request.
 */
struct GetAssetTagRequest
{
    uint8_t offset; //!< Offset to read.
    uint8_t bytes;  //!< Number of bytes to read.
} __attribute__((packed));

/** @struct GetAssetTagResponse
 *
 *  DCMI payload for Get Asset Tag command response.
 */
struct GetAssetTagResponse
{
    uint8_t tagLength; //!< Total asset tag length.
} __attribute__((packed));

/** @struct SetAssetTagRequest
 *
 *  DCMI payload for Set Asset Tag command request.
 */
struct SetAssetTagRequest
{
    uint8_t offset; //!< Offset to write.
    uint8_t bytes;  //!< Number of bytes to write.
} __attribute__((packed));

/** @struct SetAssetTagResponse
 *
 *  DCMI payload for Set Asset Tag command response.
 */
struct SetAssetTagResponse
{
    uint8_t tagLength; //!< Total asset tag length.
} __attribute__((packed));

/** @brief Check whether DCMI power management is supported
 *         in the DCMI Capabilities config file.
 *
 *  @return True if DCMI power management is supported
 */
bool isDCMIPowerMgmtSupported();

/** @brief Read the object tree to fetch the object path that implemented the
 *         Asset tag interface.
 *
 *  @param[in,out] objectTree - object tree
 *
 *  @return On success return the object tree with the object path that
 *          implemented the AssetTag interface.
 */
void readAssetTagObjectTree(dcmi::assettag::ObjectTree& objectTree);

/** @brief Read the asset tag of the server
 *
 *  @return On success return the asset tag.
 */
std::string readAssetTag();

/** @brief Write the asset tag to the asset tag DBUS property
 *
 *  @param[in] assetTag - Asset Tag to be written to the property.
 */
void writeAssetTag(const std::string& assetTag);

/** @brief Read the current power cap value
 *
 *  @param[in] bus - dbus connection
 *
 *  @return On success return the power cap value.
 */
uint32_t getPcap(sdbusplus::bus::bus& bus);

/** @brief Check if the power capping is enabled
 *
 *  @param[in] bus - dbus connection
 *
 *  @return true if the powerCap is enabled and false if the powercap
 *          is disabled.
 */
bool getPcapEnabled(sdbusplus::bus::bus& bus);

/** @struct GetPowerLimitResponse
 *
 *  DCMI payload for Get Power Limit command response.
 */
struct GetPowerLimitResponse
{
    uint16_t reserved;       //!< Reserved.
    uint8_t exceptionAction; //!< Exception action.
    uint16_t powerLimit;     //!< Power limit requested in watts.
    uint32_t correctionTime; //!< Correction time limit in milliseconds.
    uint16_t reserved1;      //!< Reserved.
    uint16_t samplingPeriod; //!< Statistics sampling period in seconds.
} __attribute__((packed));

/** @brief Set the power cap value
 *
 *  @param[in] bus - dbus connection
 *  @param[in] powerCap - power cap value
 */
void setPcap(sdbusplus::bus::bus& bus, const uint32_t powerCap);

/** @struct SetPowerLimitRequest
 *
 *  DCMI payload for Set Power Limit command request.
 */
struct SetPowerLimitRequest
{
    uint16_t reserved;       //!< Reserved
    uint8_t reserved1;       //!< Reserved
    uint8_t exceptionAction; //!< Exception action.
    uint16_t powerLimit;     //!< Power limit requested in watts.
    uint32_t correctionTime; //!< Correction time limit in milliseconds.
    uint16_t reserved2;      //!< Reserved.
    uint16_t samplingPeriod; //!< Statistics sampling period in seconds.
} __attribute__((packed));

/** @brief Enable or disable the power capping
 *
 *  @param[in] bus - dbus connection
 *  @param[in] enabled - enable/disable
 */
void setPcapEnable(sdbusplus::bus::bus& bus, bool enabled);

/** @struct ApplyPowerLimitRequest
 *
 *  DCMI payload for Activate/Deactivate Power Limit command request.
 */
struct ApplyPowerLimitRequest
{
    uint8_t powerLimitAction; //!< Power limit activation
    uint16_t reserved;        //!< Reserved
} __attribute__((packed));

/** @struct GetMgmntCtrlIdStrRequest
 *
 *  DCMI payload for Get Management Controller Identifier String cmd request.
 */
struct GetMgmntCtrlIdStrRequest
{
    uint8_t offset; //!< Offset to read.
    uint8_t bytes;  //!< Number of bytes to read.
} __attribute__((packed));

/** @struct GetMgmntCtrlIdStrResponse
 *
 *  DCMI payload for Get Management Controller Identifier String cmd response.
 */
struct GetMgmntCtrlIdStrResponse
{
    uint8_t strLen; //!< ID string length.
    char data[];    //!< ID string
} __attribute__((packed));

/** @struct SetMgmntCtrlIdStrRequest
 *
 *  DCMI payload for Set Management Controller Identifier String cmd request.
 */
struct SetMgmntCtrlIdStrRequest
{
    uint8_t offset; //!< Offset to write.
    uint8_t bytes;  //!< Number of bytes to read.
    char data[];    //!< ID string
} __attribute__((packed));

/** @struct GetMgmntCtrlIdStrResponse
 *
 *  DCMI payload for Get Management Controller Identifier String cmd response.
 */
struct SetMgmntCtrlIdStrResponse
{
    uint8_t offset; //!< Last Offset Written.
} __attribute__((packed));

/** @enum DCMICapParameters
 *
 * DCMI Capability parameters
 */
enum class DCMICapParameters
{
    SUPPORTED_DCMI_CAPS = 0x01,             //!< Supported DCMI Capabilities
    MANDATORY_PLAT_ATTRIBUTES = 0x02,       //!< Mandatory Platform Attributes
    OPTIONAL_PLAT_ATTRIBUTES = 0x03,        //!< Optional Platform Attributes
    MANAGEABILITY_ACCESS_ATTRIBUTES = 0x04, //!< Manageability Access Attributes
};

/** @struct GetDCMICapRequest
 *
 *  DCMI payload for Get capabilities cmd request.
 */
struct GetDCMICapRequest
{
    uint8_t param; //!< Capability parameter selector.
} __attribute__((packed));

/** @struct GetDCMICapRequest
 *
 *  DCMI payload for Get capabilities cmd response.
 */
struct GetDCMICapResponse
{
    uint8_t major;         //!< DCMI Specification Conformance - major ver
    uint8_t minor;         //!< DCMI Specification Conformance - minor ver
    uint8_t paramRevision; //!< Parameter Revision = 02h
    uint8_t data[];        //!< Capability array
} __attribute__((packed));

/** @struct DCMICap
 *
 *  DCMI capabilities protocol info.
 */
struct DCMICap
{
    std::string name;     //!< Name of DCMI capability.
    uint8_t bytePosition; //!< Starting byte number from DCMI spec.
    uint8_t position;     //!< bit position from the DCMI spec.
    uint8_t length;       //!< Length of the value from DCMI spec.
};

using DCMICapList = std::vector<DCMICap>;

/** @struct DCMICapEntry
 *
 *  DCMI capabilities list and size for each parameter.
 */
struct DCMICapEntry
{
    uint8_t size;        //!< Size of capability array in bytes.
    DCMICapList capList; //!< List of capabilities for a parameter.
};

using DCMICaps = std::map<DCMICapParameters, DCMICapEntry>;

/** @struct GetTempReadingsRequest
 *
 *  DCMI payload for Get Temperature Readings request
 */
struct GetTempReadingsRequest
{
    uint8_t sensorType;     //!< Type of the sensor
    uint8_t entityId;       //!< Entity ID
    uint8_t entityInstance; //!< Entity Instance (0 means all instances)
    uint8_t instanceStart;  //!< Instance start (used if instance is 0)
} __attribute__((packed));

/** @struct GetTempReadingsResponse
 *
 *  DCMI header for Get Temperature Readings response
 */
struct GetTempReadingsResponseHdr
{
    uint8_t numInstances; //!< No. of instances for requested id
    uint8_t numDataSets;  //!< No. of sets of temperature data
} __attribute__((packed));

/** @brief Parse out JSON config file.
 *
 *  @param[in] configFile - JSON config file name
 *
 *  @return A json object
 */
Json parseJSONConfig(const std::string& configFile);

namespace temp_readings
{
/** @brief Read temperature from a d-bus object, scale it as per dcmi
 *         get temperature reading requirements.
 *
 *  @param[in] dbusService - the D-Bus service
 *  @param[in] dbusPath - the D-Bus path
 *
 *  @return A temperature reading
 */
Temperature readTemp(const std::string& dbusService,
                     const std::string& dbusPath);

/** @brief Read temperatures and fill up DCMI response for the Get
 *         Temperature Readings command. This looks at a specific
 *         instance.
 *
 *  @param[in] type - one of "inlet", "cpu", "baseboard"
 *  @param[in] instance - A non-zero Entity instance number
 *
 *  @return A tuple, containing a temperature reading and the
 *          number of instances.
 */
std::tuple<Response, NumInstances> read(const std::string& type,
                                        uint8_t instance);

/** @brief Read temperatures and fill up DCMI response for the Get
 *         Temperature Readings command. This looks at a range of
 *         instances.
 *
 *  @param[in] type - one of "inlet", "cpu", "baseboard"
 *  @param[in] instanceStart - Entity instance start index
 *
 *  @return A tuple, containing a list of temperature readings and the
 *          number of instances.
 */
std::tuple<ResponseList, NumInstances> readAll(const std::string& type,
                                               uint8_t instanceStart);
} // namespace temp_readings

namespace sensor_info
{
/** @brief Create response from JSON config.
 *
 *  @param[in] config - JSON config info about DCMI sensors
 *
 *  @return Sensor info response
 */
Response createFromJson(const Json& config);

/** @brief Read sensor info and fill up DCMI response for the Get
 *         Sensor Info command. This looks at a specific
 *         instance.
 *
 *  @param[in] type - one of "inlet", "cpu", "baseboard"
 *  @param[in] instance - A non-zero Entity instance number
 *  @param[in] config - JSON config info about DCMI sensors
 *
 *  @return A tuple, containing a sensor info response and
 *          number of instances.
 */
std::tuple<Response, NumInstances> read(const std::string& type,
                                        uint8_t instance, const Json& config);

/** @brief Read sensor info and fill up DCMI response for the Get
 *         Sensor Info command. This looks at a range of
 *         instances.
 *
 *  @param[in] type - one of "inlet", "cpu", "baseboard"
 *  @param[in] instanceStart - Entity instance start index
 *  @param[in] config - JSON config info about DCMI sensors
 *
 *  @return A tuple, containing a list of sensor info responses and the
 *          number of instances.
 */
std::tuple<ResponseList, NumInstances>
    readAll(const std::string& type, uint8_t instanceStart, const Json& config);
} // namespace sensor_info

/** @brief Read power reading from power reading sensor object
 *
 *  @param[in] bus - dbus connection
 *
 *  @return total power reading
 */
int64_t getPowerReading(sdbusplus::bus::bus& bus);

/** @struct GetPowerReadingRequest
 *
 *  DCMI Get Power Reading command request.
 *  Refer DCMI specification Version 1.1 Section 6.6.1
 */
struct GetPowerReadingRequest
{
    uint8_t mode;          //!< Mode
    uint8_t modeAttribute; //!< Mode Attributes
} __attribute__((packed));

/** @struct GetPowerReadingResponse
 *
 *  DCMI Get Power Reading command response.
 *  Refer DCMI specification Version 1.1 Section 6.6.1
 */
struct GetPowerReadingResponse
{
    uint16_t currentPower;     //!< Current power in watts
    uint16_t minimumPower;     //!< Minimum power over sampling duration
                               //!< in watts
    uint16_t maximumPower;     //!< Maximum power over sampling duration
                               //!< in watts
    uint16_t averagePower;     //!< Average power over sampling duration
                               //!< in watts
    uint32_t timeStamp;        //!< IPMI specification based time stamp
    uint32_t timeFrame;        //!< Statistics reporting time period in milli
                               //!< seconds.
    uint8_t powerReadingState; //!< Power Reading State
} __attribute__((packed));

/** @struct GetSensorInfoRequest
 *
 *  DCMI payload for Get Sensor Info request
 */
struct GetSensorInfoRequest
{
    uint8_t sensorType;     //!< Type of the sensor
    uint8_t entityId;       //!< Entity ID
    uint8_t entityInstance; //!< Entity Instance (0 means all instances)
    uint8_t instanceStart;  //!< Instance start (used if instance is 0)
} __attribute__((packed));

/** @struct GetSensorInfoResponseHdr
 *
 *  DCMI header for Get Sensor Info response
 */
struct GetSensorInfoResponseHdr
{
    uint8_t numInstances; //!< No. of instances for requested id
    uint8_t numRecords;   //!< No. of record ids in the response
} __attribute__((packed));
/**
 *  @brief Parameters for DCMI Configuration Parameters
 */
enum class DCMIConfigParameters : uint8_t
{
    ActivateDHCP = 1,
    DiscoveryConfig,
    DHCPTiming1,
    DHCPTiming2,
    DHCPTiming3,
};

/** @struct SetConfParamsRequest
 *
 *  DCMI Set DCMI Configuration Parameters Command.
 *  Refer DCMI specification Version 1.1 Section 6.1.2
 */
struct SetConfParamsRequest
{
    uint8_t paramSelect; //!< Parameter selector.
    uint8_t setSelect;   //!< Set Selector (use 00h for parameters that only
                         //!< have one set).
    uint8_t data[];      //!< Configuration parameter data.
} __attribute__((packed));

/** @struct GetConfParamsRequest
 *
 *  DCMI Get DCMI Configuration Parameters Command.
 *  Refer DCMI specification Version 1.1 Section 6.1.3
 */
struct GetConfParamsRequest
{
    uint8_t paramSelect; //!< Parameter selector.
    uint8_t setSelect;   //!< Set Selector. Selects a given set of parameters
                         //!< under a given Parameter selector value. 00h if
                         //!< parameter doesn't use a Set Selector.
} __attribute__((packed));

/** @struct GetConfParamsResponse
 *
 *  DCMI Get DCMI Configuration Parameters Command response.
 *  Refer DCMI specification Version 1.1 Section 6.1.3
 */
struct GetConfParamsResponse
{
    uint8_t major;         //!< DCMI Spec Conformance - major ver = 01h.
    uint8_t minor;         //!< DCMI Spec Conformance - minor ver = 05h.
    uint8_t paramRevision; //!< Parameter Revision = 01h.
    uint8_t data[];        //!< Parameter data.

} __attribute__((packed));

} // namespace dcmi
