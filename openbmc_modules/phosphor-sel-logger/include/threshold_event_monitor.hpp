/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once
#include <sel_logger.hpp>
#include <sensorutils.hpp>

#include <string_view>
#include <variant>

enum class thresholdEventOffsets : uint8_t
{
    lowerNonCritGoingLow = 0x00,
    lowerCritGoingLow = 0x02,
    upperNonCritGoingHigh = 0x07,
    upperCritGoingHigh = 0x09,
};

static constexpr const uint8_t thresholdEventDataTriggerReadingByte2 = (1 << 6);
static constexpr const uint8_t thresholdEventDataTriggerReadingByte3 = (1 << 4);

static const std::string openBMCMessageRegistryVersion("0.1");

inline static sdbusplus::bus::match::match startThresholdAssertMonitor(
    std::shared_ptr<sdbusplus::asio::connection> conn)
{
    auto thresholdAssertMatcherCallback = [conn](sdbusplus::message::message&
                                                     msg) {
        // This static set of std::pair<path, event> tracks asserted events to
        // avoid duplicate logs or deasserts logged without an assert
        static boost::container::flat_set<std::pair<std::string, std::string>>
            assertedEvents;
        std::vector<uint8_t> eventData(selEvtDataMaxSize,
                                       selEvtDataUnspecified);

        // Get the event type and assertion details from the message
        std::string sensorName;
        std::string thresholdInterface;
        std::string event;
        bool assert;
        double assertValue;
        try
        {
            msg.read(sensorName, thresholdInterface, event, assert,
                     assertValue);
        }
        catch (sdbusplus::exception_t&)
        {
            std::cerr << "error getting assert signal data from "
                      << msg.get_path() << "\n";
            return;
        }

        // Check the asserted events to determine if we should log this event
        std::pair<std::string, std::string> pathAndEvent(
            std::string(msg.get_path()), event);
        if (assert)
        {
            // For asserts, add the event to the set and only log it if it's new
            if (assertedEvents.insert(pathAndEvent).second == false)
            {
                // event is already in the set
                return;
            }
        }
        else
        {
            // For deasserts, remove the event and only log the deassert if it
            // was asserted
            if (assertedEvents.erase(pathAndEvent) == 0)
            {
                // asserted event was not in the set
                return;
            }
        }

        // Set the IPMI threshold event type based on the event details from the
        // message
        if (event == "CriticalAlarmLow")
        {
            eventData[0] =
                static_cast<uint8_t>(thresholdEventOffsets::lowerCritGoingLow);
        }
        else if (event == "WarningAlarmLow")
        {
            eventData[0] = static_cast<uint8_t>(
                thresholdEventOffsets::lowerNonCritGoingLow);
        }
        else if (event == "WarningAlarmHigh")
        {
            eventData[0] = static_cast<uint8_t>(
                thresholdEventOffsets::upperNonCritGoingHigh);
        }
        else if (event == "CriticalAlarmHigh")
        {
            eventData[0] =
                static_cast<uint8_t>(thresholdEventOffsets::upperCritGoingHigh);
        }
        // Indicate that bytes 2 and 3 are threshold sensor trigger values
        eventData[0] |= thresholdEventDataTriggerReadingByte2 |
                        thresholdEventDataTriggerReadingByte3;

        // Get the sensor reading to put in the event data
        sdbusplus::message::message getSensorValue =
            conn->new_method_call(msg.get_sender(), msg.get_path(),
                                  "org.freedesktop.DBus.Properties", "GetAll");
        getSensorValue.append("xyz.openbmc_project.Sensor.Value");
        boost::container::flat_map<std::string, std::variant<double, int64_t>>
            sensorValue;
        try
        {
            sdbusplus::message::message getSensorValueResp =
                conn->call(getSensorValue);
            getSensorValueResp.read(sensorValue);
        }
        catch (sdbusplus::exception_t&)
        {
            std::cerr << "error getting sensor value from " << msg.get_path()
                      << "\n";
            return;
        }
        double max = 0;
        auto findMax = sensorValue.find("MaxValue");
        if (findMax != sensorValue.end())
        {
            max = std::visit(ipmi::VariantToDoubleVisitor(), findMax->second);
        }
        double min = 0;
        auto findMin = sensorValue.find("MinValue");
        if (findMin != sensorValue.end())
        {
            min = std::visit(ipmi::VariantToDoubleVisitor(), findMin->second);
        }

        try
        {
            eventData[1] = ipmi::getScaledIPMIValue(assertValue, max, min);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what();
            eventData[1] = selEvtDataUnspecified;
        }

        // Get the threshold value to put in the event data
        // Get the threshold parameter by removing the "Alarm" text from the
        // event string
        std::string alarm("Alarm");
        if (std::string::size_type pos = event.find(alarm);
            pos != std::string::npos)
        {
            event.erase(pos, alarm.length());
        }
        sdbusplus::message::message getThreshold =
            conn->new_method_call(msg.get_sender(), msg.get_path(),
                                  "org.freedesktop.DBus.Properties", "Get");
        getThreshold.append(thresholdInterface, event);
        std::variant<double, int64_t> thresholdValue;
        try
        {
            sdbusplus::message::message getThresholdResp =
                conn->call(getThreshold);
            getThresholdResp.read(thresholdValue);
        }
        catch (sdbusplus::exception_t&)
        {
            std::cerr << "error getting sensor threshold from "
                      << msg.get_path() << "\n";
            return;
        }
        double thresholdVal =
            std::visit(ipmi::VariantToDoubleVisitor(), thresholdValue);

        double scale = 0;
        auto findScale = sensorValue.find("Scale");
        if (findScale != sensorValue.end())
        {
            scale =
                std::visit(ipmi::VariantToDoubleVisitor(), findScale->second);
            thresholdVal *= std::pow(10, scale);
        }
        try
        {
            eventData[2] = ipmi::getScaledIPMIValue(thresholdVal, max, min);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what();
            eventData[2] = selEvtDataUnspecified;
        }

        std::string threshold;
        std::string direction;
        std::string redfishMessageID =
            "OpenBMC." + openBMCMessageRegistryVersion;
        if (event == "CriticalLow")
        {
            threshold = "critical low";
            if (assert)
            {
                direction = "low";
                redfishMessageID += ".SensorThresholdCriticalLowGoingLow";
            }
            else
            {
                direction = "high";
                redfishMessageID += ".SensorThresholdCriticalLowGoingHigh";
            }
        }
        else if (event == "WarningLow")
        {
            threshold = "warning low";
            if (assert)
            {
                direction = "low";
                redfishMessageID += ".SensorThresholdWarningLowGoingLow";
            }
            else
            {
                direction = "high";
                redfishMessageID += ".SensorThresholdWarningLowGoingHigh";
            }
        }
        else if (event == "WarningHigh")
        {
            threshold = "warning high";
            if (assert)
            {
                direction = "high";
                redfishMessageID += ".SensorThresholdWarningHighGoingHigh";
            }
            else
            {
                direction = "low";
                redfishMessageID += ".SensorThresholdWarningHighGoingLow";
            }
        }
        else if (event == "CriticalHigh")
        {
            threshold = "critical high";
            if (assert)
            {
                direction = "high";
                redfishMessageID += ".SensorThresholdCriticalHighGoingHigh";
            }
            else
            {
                direction = "low";
                redfishMessageID += ".SensorThresholdCriticalHighGoingLow";
            }
        }

        std::string journalMsg(std::string(sensorName) + " sensor crossed a " +
                               threshold + " threshold going " + direction +
                               ". Reading=" + std::to_string(assertValue) +
                               " Threshold=" + std::to_string(thresholdVal) +
                               ".");

        selAddSystemRecord(
            journalMsg, std::string(msg.get_path()), eventData, assert,
            selBMCGenID, "REDFISH_MESSAGE_ID=%s", redfishMessageID.c_str(),
            "REDFISH_MESSAGE_ARGS=%.*s,%f,%f", sensorName.length(),
            sensorName.data(), assertValue, thresholdVal);
    };
    sdbusplus::bus::match::match thresholdAssertMatcher(
        static_cast<sdbusplus::bus::bus&>(*conn),
        "type='signal', member='ThresholdAsserted'",
        std::move(thresholdAssertMatcherCallback));
    return thresholdAssertMatcher;
}
