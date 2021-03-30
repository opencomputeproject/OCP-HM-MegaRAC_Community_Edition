#pragma once

#include "config.h"

#include "sensorhandler.hpp"

#include <cmath>
#include <ipmid/api.hpp>
#include <ipmid/types.hpp>
#include <ipmid/utils.hpp>
#include <sdbusplus/message/types.hpp>

namespace ipmi
{
namespace sensor
{

using Assertion = uint16_t;
using Deassertion = uint16_t;
using AssertionSet = std::pair<Assertion, Deassertion>;

using Service = std::string;
using Path = std::string;
using Interface = std::string;

using ServicePath = std::pair<Path, Service>;

using Interfaces = std::vector<Interface>;

using MapperResponseType = std::map<Path, std::map<Service, Interfaces>>;

/** @brief get the D-Bus service and service path
 *  @param[in] bus - The Dbus bus object
 *  @param[in] interface - interface to the service
 *  @param[in] path - interested path in the list of objects
 *  @return pair of service path and service
 */
ServicePath getServiceAndPath(sdbusplus::bus::bus& bus,
                              const std::string& interface,
                              const std::string& path = std::string());

/** @brief Make assertion set from input data
 *  @param[in] cmdData - Input sensor data
 *  @return pair of assertion and deassertion set
 */
AssertionSet getAssertionSet(const SetSensorReadingReq& cmdData);

/** @brief send the message to DBus
 *  @param[in] msg - message to send
 *  @return failure status in IPMI error code
 */
ipmi_ret_t updateToDbus(IpmiUpdateData& msg);

namespace get
{

/** @brief Populate sensor name from the D-Bus property associated with the
 *         sensor. In the example entry from the yaml, the name of the D-bus
 *         property "AttemptsLeft" is the sensor name.
 *
 *         0x07:
 *            sensorType: 195
 *            path: /xyz/openbmc_project/state/host0
 *            sensorReadingType: 0x6F
 *            serviceInterface: org.freedesktop.DBus.Properties
 *            readingType: readingAssertion
 *            sensorNamePattern: nameProperty
 *            interfaces:
 *              xyz.openbmc_project.Control.Boot.RebootAttempts:
 *                AttemptsLeft:
 *                    Offsets:
 *                        0xFF:
 *                          type: uint32_t
 *
 *
 *  @param[in] sensorInfo - Dbus info related to sensor.
 *
 *  @return On success return the sensor name for the sensor.
 */
inline SensorName nameProperty(const Info& sensorInfo)
{
    return sensorInfo.propertyInterfaces.begin()->second.begin()->first;
}

/** @brief Populate sensor name from the D-Bus object associated with the
 *         sensor. If the object path is /system/chassis/motherboard/dimm0 then
 *         the leaf dimm0 is considered as the sensor name.
 *
 *  @param[in] sensorInfo - Dbus info related to sensor.
 *
 *  @return On success return the sensor name for the sensor.
 */
inline SensorName nameLeaf(const Info& sensorInfo)
{
    return sensorInfo.sensorPath.substr(
        sensorInfo.sensorPath.find_last_of('/') + 1,
        sensorInfo.sensorPath.length());
}

/** @brief Populate sensor name from the D-Bus object associated with the
 *         sensor. If the object path is /system/chassis/motherboard/cpu0/core0
 *         then the sensor name is cpu0_core0. The leaf and the parent is put
 *         together to get the sensor name.
 *
 *  @param[in] sensorInfo - Dbus info related to sensor.
 *
 *  @return On success return the sensor name for the sensor.
 */
SensorName nameParentLeaf(const Info& sensorInfo);

/**
 *  @brief Helper function to map the dbus info to sensor's assertion status
 *         for the get sensor reading command.
 *
 *  @param[in] sensorInfo - Dbus info related to sensor.
 *  @param[in] path - Dbus object path.
 *  @param[in] interface - Dbus interface.
 *
 *  @return Response for get sensor reading command.
 */
GetSensorResponse mapDbusToAssertion(const Info& sensorInfo,
                                     const InstancePath& path,
                                     const DbusInterface& interface);

/**
 *  @brief Map the Dbus info to sensor's assertion status in the Get sensor
 *         reading command response.
 *
 *  @param[in] sensorInfo - Dbus info related to sensor.
 *
 *  @return Response for get sensor reading command.
 */
GetSensorResponse assertion(const Info& sensorInfo);

/**
 *  @brief Maps the Dbus info to the reading field in the Get sensor reading
 *         command response.
 *
 *  @param[in] sensorInfo - Dbus info related to sensor.
 *
 *  @return Response for get sensor reading command.
 */
GetSensorResponse eventdata2(const Info& sensorInfo);

/**
 *  @brief readingAssertion is a case where the entire assertion state field
 *         serves as the sensor value.
 *
 *  @tparam T - type of the dbus property related to sensor.
 *  @param[in] sensorInfo - Dbus info related to sensor.
 *
 *  @return Response for get sensor reading command.
 */
template <typename T>
GetSensorResponse readingAssertion(const Info& sensorInfo)
{
    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};
    GetSensorResponse response{};

    enableScanning(&response);

    auto service = ipmi::getService(bus, sensorInfo.sensorInterface,
                                    sensorInfo.sensorPath);

    auto propValue = ipmi::getDbusProperty(
        bus, service, sensorInfo.sensorPath,
        sensorInfo.propertyInterfaces.begin()->first,
        sensorInfo.propertyInterfaces.begin()->second.begin()->first);

    setAssertionBytes(static_cast<uint16_t>(std::get<T>(propValue)), &response);

    return response;
}

/** @brief Map the Dbus info to the reading field in the Get sensor reading
 *         command response
 *
 *  @tparam T - type of the dbus property related to sensor.
 *  @param[in] sensorInfo - Dbus info related to sensor.
 *
 *  @return Response for get sensor reading command.
 */
template <typename T>
GetSensorResponse readingData(const Info& sensorInfo)
{
    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};

    GetSensorResponse response{};

    enableScanning(&response);

    auto service = ipmi::getService(bus, sensorInfo.sensorInterface,
                                    sensorInfo.sensorPath);

#ifdef UPDATE_FUNCTIONAL_ON_FAIL
    // Check the OperationalStatus interface for functional property
    if (sensorInfo.propertyInterfaces.begin()->first ==
        "xyz.openbmc_project.Sensor.Value")
    {
        bool functional = true;
        try
        {
            auto funcValue = ipmi::getDbusProperty(
                bus, service, sensorInfo.sensorPath,
                "xyz.openbmc_project.State.Decorator.OperationalStatus",
                "Functional");
            functional = std::get<bool>(funcValue);
        }
        catch (...)
        {
            // No-op if Functional property could not be found since this
            // check is only valid for Sensor.Value read for hwmonio
        }
        if (!functional)
        {
            throw SensorFunctionalError();
        }
    }
#endif

    auto propValue = ipmi::getDbusProperty(
        bus, service, sensorInfo.sensorPath,
        sensorInfo.propertyInterfaces.begin()->first,
        sensorInfo.propertyInterfaces.begin()->second.begin()->first);

    double value = std::get<T>(propValue) *
                   std::pow(10, sensorInfo.scale - sensorInfo.exponentR);

    auto rawData = static_cast<uint8_t>((value - sensorInfo.scaledOffset) /
                                        sensorInfo.coefficientM);
    setReading(rawData, &response);

    return response;
}

} // namespace get

namespace set
{

/** @brief Make a DBus message for a Dbus call
 *  @param[in] updateInterface - Interface name
 *  @param[in] sensorPath - Path of the sensor
 *  @param[in] command - command to be executed
 *  @param[in] sensorInterface - DBus interface of sensor
 *  @return a dbus message
 */
IpmiUpdateData makeDbusMsg(const std::string& updateInterface,
                           const std::string& sensorPath,
                           const std::string& command,
                           const std::string& sensorInterface);

/** @brief Update d-bus based on assertion type sensor data
 *  @param[in] cmdData - input sensor data
 *  @param[in] sensorInfo - sensor d-bus info
 *  @return a IPMI error code
 */
ipmi_ret_t assertion(const SetSensorReadingReq& cmdData,
                     const Info& sensorInfo);

/** @brief Update d-bus based on a reading assertion
 *  @tparam T - type of d-bus property mapping this sensor
 *  @param[in] cmdData - input sensor data
 *  @param[in] sensorInfo - sensor d-bus info
 *  @return a IPMI error code
 */
template <typename T>
ipmi_ret_t readingAssertion(const SetSensorReadingReq& cmdData,
                            const Info& sensorInfo)
{
    auto msg =
        makeDbusMsg("org.freedesktop.DBus.Properties", sensorInfo.sensorPath,
                    "Set", sensorInfo.sensorInterface);

    const auto& interface = sensorInfo.propertyInterfaces.begin();
    msg.append(interface->first);
    for (const auto& property : interface->second)
    {
        msg.append(property.first);
        std::variant<T> value = static_cast<T>((cmdData.assertOffset8_14 << 8) |
                                               cmdData.assertOffset0_7);
        msg.append(value);
    }
    return updateToDbus(msg);
}

/** @brief Update d-bus based on a discrete reading
 *  @param[in] cmdData - input sensor data
 *  @param[in] sensorInfo - sensor d-bus info
 *  @return an IPMI error code
 */
template <typename T>
ipmi_ret_t readingData(const SetSensorReadingReq& cmdData,
                       const Info& sensorInfo)
{
    T raw_value =
        (sensorInfo.coefficientM * cmdData.reading) + sensorInfo.scaledOffset;

    raw_value *= std::pow(10, sensorInfo.exponentR - sensorInfo.scale);

    auto msg =
        makeDbusMsg("org.freedesktop.DBus.Properties", sensorInfo.sensorPath,
                    "Set", sensorInfo.sensorInterface);

    const auto& interface = sensorInfo.propertyInterfaces.begin();
    msg.append(interface->first);

    for (const auto& property : interface->second)
    {
        msg.append(property.first);
        std::variant<T> value = raw_value;
        msg.append(value);
    }
    return updateToDbus(msg);
}

/** @brief Update d-bus based on eventdata type sensor data
 *  @param[in] cmdData - input sensor data
 *  @param[in] sensorInfo - sensor d-bus info
 *  @return a IPMI error code
 */
ipmi_ret_t eventdata(const SetSensorReadingReq& cmdData, const Info& sensorInfo,
                     uint8_t data);

/** @brief Update d-bus based on eventdata1 type sensor data
 *  @param[in] cmdData - input sensor data
 *  @param[in] sensorInfo - sensor d-bus info
 *  @return a IPMI error code
 */
inline ipmi_ret_t eventdata1(const SetSensorReadingReq& cmdData,
                             const Info& sensorInfo)
{
    return eventdata(cmdData, sensorInfo, cmdData.eventData1);
}

/** @brief Update d-bus based on eventdata2 type sensor data
 *  @param[in] cmdData - input sensor data
 *  @param[in] sensorInfo - sensor d-bus info
 *  @return a IPMI error code
 */
inline ipmi_ret_t eventdata2(const SetSensorReadingReq& cmdData,
                             const Info& sensorInfo)
{
    return eventdata(cmdData, sensorInfo, cmdData.eventData2);
}

/** @brief Update d-bus based on eventdata3 type sensor data
 *  @param[in] cmdData - input sensor data
 *  @param[in] sensorInfo - sensor d-bus info
 *  @return a IPMI error code
 */
inline ipmi_ret_t eventdata3(const SetSensorReadingReq& cmdData,
                             const Info& sensorInfo)
{
    return eventdata(cmdData, sensorInfo, cmdData.eventData3);
}

} // namespace set

namespace notify
{

/** @brief Make a DBus message for a Dbus call
 *  @param[in] updateInterface - Interface name
 *  @param[in] sensorPath - Path of the sensor
 *  @param[in] command - command to be executed
 *  @param[in] sensorInterface - DBus interface of sensor
 *  @return a dbus message
 */
IpmiUpdateData makeDbusMsg(const std::string& updateInterface,
                           const std::string& sensorPath,
                           const std::string& command,
                           const std::string& sensorInterface);

/** @brief Update d-bus based on assertion type sensor data
 *  @param[in] interfaceMap - sensor interface
 *  @param[in] cmdData - input sensor data
 *  @param[in] sensorInfo - sensor d-bus info
 *  @return a IPMI error code
 */
ipmi_ret_t assertion(const SetSensorReadingReq& cmdData,
                     const Info& sensorInfo);

} // namespace notify

namespace inventory
{

namespace get
{

/**
 *  @brief Map the Dbus info to sensor's assertion status in the Get sensor
 *         reading command response.
 *
 *  @param[in] sensorInfo - Dbus info related to sensor.
 *
 *  @return Response for get sensor reading command.
 */
GetSensorResponse assertion(const Info& sensorInfo);

} // namespace get

} // namespace inventory
} // namespace sensor
} // namespace ipmi
