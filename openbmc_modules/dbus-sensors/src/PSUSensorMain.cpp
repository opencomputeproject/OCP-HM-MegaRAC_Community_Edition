/*
// Copyright (c) 2019 Intel Corporation
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

#include "PSUEvent.hpp"
#include "PSUSensor.hpp"
#include "Utils.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <string>
#include <utility>
#include <variant>
#include <vector>

static constexpr bool DEBUG = false;

static constexpr std::array<const char*, 13> sensorTypes = {
    "xyz.openbmc_project.Configuration.ADM1272",
    "xyz.openbmc_project.Configuration.ADM1278",
    "xyz.openbmc_project.Configuration.INA219",
    "xyz.openbmc_project.Configuration.INA230",
    "xyz.openbmc_project.Configuration.ISL68137",
    "xyz.openbmc_project.Configuration.MAX16601",
    "xyz.openbmc_project.Configuration.MAX20730",
    "xyz.openbmc_project.Configuration.MAX20734",
    "xyz.openbmc_project.Configuration.MAX20796",
    "xyz.openbmc_project.Configuration.MAX34451",
    "xyz.openbmc_project.Configuration.pmbus",
    "xyz.openbmc_project.Configuration.PXE1610",
    "xyz.openbmc_project.Configuration.RAA228228"};

static std::vector<std::string> pmbusNames = {
    "adm1272",  "adm1278",  "ina219",   "ina230",   "isl68137",
    "max16601", "max20730", "max20734", "max20796", "max34451",
    "pmbus",    "pxe1610",  "raa228228"};

namespace fs = std::filesystem;

static boost::container::flat_map<std::string, std::shared_ptr<PSUSensor>>
    sensors;
static boost::container::flat_map<std::string, std::unique_ptr<PSUCombineEvent>>
    combineEvents;
static boost::container::flat_map<std::string, std::unique_ptr<PwmSensor>>
    pwmSensors;
static boost::container::flat_map<std::string, std::string> sensorTable;
static boost::container::flat_map<std::string, PSUProperty> labelMatch;
static boost::container::flat_map<std::string, std::string> pwmTable;
static boost::container::flat_map<std::string, std::vector<std::string>>
    eventMatch;
static boost::container::flat_map<
    std::string,
    boost::container::flat_map<std::string, std::vector<std::string>>>
    groupEventMatch;
static boost::container::flat_map<std::string, std::vector<std::string>>
    limitEventMatch;

static std::vector<PSUProperty> psuProperties;

// Function CheckEvent will check each attribute from eventMatch table in the
// sysfs. If the attributes exists in sysfs, then store the complete path
// of the attribute into eventPathList.
void checkEvent(
    const std::string& directory,
    const boost::container::flat_map<std::string, std::vector<std::string>>&
        eventMatch,
    boost::container::flat_map<std::string, std::vector<std::string>>&
        eventPathList)
{
    for (const auto& match : eventMatch)
    {
        const std::vector<std::string>& eventAttrs = match.second;
        const std::string& eventName = match.first;
        for (const auto& eventAttr : eventAttrs)
        {
            auto eventPath = directory + "/" + eventAttr;

            std::ifstream eventFile(eventPath);
            if (!eventFile.good())
            {
                continue;
            }

            eventPathList[eventName].push_back(eventPath);
        }
    }
}

// Check Group Events which contains more than one targets in each combine
// events.
void checkGroupEvent(
    const std::string& directory,
    const boost::container::flat_map<
        std::string,
        boost::container::flat_map<std::string, std::vector<std::string>>>&
        groupEventMatch,
    boost::container::flat_map<
        std::string,
        boost::container::flat_map<std::string, std::vector<std::string>>>&
        groupEventPathList)
{
    for (const auto& match : groupEventMatch)
    {
        const std::string& groupEventName = match.first;
        const boost::container::flat_map<std::string, std::vector<std::string>>
            events = match.second;
        boost::container::flat_map<std::string, std::vector<std::string>>
            pathList;
        for (const auto& match : events)
        {
            const std::string& eventName = match.first;
            const std::vector<std::string>& eventAttrs = match.second;
            for (const auto& eventAttr : eventAttrs)
            {
                auto eventPath = directory + "/" + eventAttr;
                std::ifstream eventFile(eventPath);
                if (!eventFile.good())
                {
                    continue;
                }

                pathList[eventName].push_back(eventPath);
            }
        }
        groupEventPathList[groupEventName] = pathList;
    }
}

// Function checkEventLimits will check all the psu related xxx_input attributes
// in sysfs to see if xxx_crit_alarm xxx_lcrit_alarm xxx_max_alarm
// xxx_min_alarm exist, then store the existing paths of the alarm attributes
// to eventPathList.
void checkEventLimits(
    const std::string& sensorPathStr,
    const boost::container::flat_map<std::string, std::vector<std::string>>&
        limitEventMatch,
    boost::container::flat_map<std::string, std::vector<std::string>>&
        eventPathList)
{
    for (const auto& limitMatch : limitEventMatch)
    {
        const std::vector<std::string>& limitEventAttrs = limitMatch.second;
        const std::string& eventName = limitMatch.first;
        for (const auto& limitEventAttr : limitEventAttrs)
        {
            auto limitEventPath =
                boost::replace_all_copy(sensorPathStr, "input", limitEventAttr);
            std::ifstream eventFile(limitEventPath);
            if (!eventFile.good())
            {
                continue;
            }
            eventPathList[eventName].push_back(limitEventPath);
        }
    }
}

static void
    checkPWMSensor(const fs::path& sensorPath, std::string& labelHead,
                   const std::string& interfacePath,
                   std::shared_ptr<sdbusplus::asio::connection>& dbusConnection,
                   sdbusplus::asio::object_server& objectServer,
                   const std::string& psuName)
{
    for (const auto& pwmName : pwmTable)
    {
        if (pwmName.first != labelHead)
        {
            continue;
        }

        const std::string& sensorPathStr = sensorPath.string();
        const std::string& pwmPathStr =
            boost::replace_all_copy(sensorPathStr, "input", "target");
        std::ifstream pwmFile(pwmPathStr);
        if (!pwmFile.good())
        {
            continue;
        }

        auto findPWMSensor = pwmSensors.find(psuName + labelHead);
        if (findPWMSensor != pwmSensors.end())
        {
            continue;
        }

        pwmSensors[psuName + labelHead] = std::make_unique<PwmSensor>(
            "Pwm_" + psuName + "_" + pwmName.second, pwmPathStr, dbusConnection,
            objectServer, interfacePath + "_" + pwmName.second, "PSU");
    }
}

void createSensors(boost::asio::io_service& io,
                   sdbusplus::asio::object_server& objectServer,
                   std::shared_ptr<sdbusplus::asio::connection>& dbusConnection)
{

    ManagedObjectType sensorConfigs;
    int numCreated = 0;
    bool useCache = false;

    // TODO may need only modify the ones that need to be changed.
    sensors.clear();
    for (const char* type : sensorTypes)
    {
        if (!getSensorConfiguration(type, dbusConnection, sensorConfigs,
                                    useCache))
        {
            std::cerr << "error get sensor config from entity manager\n";
            return;
        }
        useCache = true;
    }

    std::vector<fs::path> pmbusPaths;
    if (!findFiles(fs::path("/sys/class/hwmon"), "name", pmbusPaths))
    {
        std::cerr << "No PSU sensors in system\n";
        return;
    }

    boost::container::flat_set<std::string> directories;
    for (const auto& pmbusPath : pmbusPaths)
    {
        boost::container::flat_map<std::string, std::vector<std::string>>
            eventPathList;
        boost::container::flat_map<
            std::string,
            boost::container::flat_map<std::string, std::vector<std::string>>>
            groupEventPathList;

        std::ifstream nameFile(pmbusPath);
        if (!nameFile.good())
        {
            std::cerr << "Failure finding pmbus path " << pmbusPath << "\n";
            continue;
        }

        std::string pmbusName;
        std::getline(nameFile, pmbusName);
        nameFile.close();

        if (std::find(pmbusNames.begin(), pmbusNames.end(), pmbusName) ==
            pmbusNames.end())
        {
            // To avoid this error message, add your driver name to
            // the pmbusNames vector at the top of this file.
            std::cerr << "Driver name " << pmbusName
                      << " not found in sensor whitelist\n";
            continue;
        }

        const std::string* psuName;
        auto directory = pmbusPath.parent_path();

        auto ret = directories.insert(directory.string());
        if (!ret.second)
        {
            std::cerr << "Duplicate path " << directory.string() << "\n";
            continue; // check if path has already been searched
        }

        fs::path device = directory / "device";
        std::string deviceName = fs::canonical(device).stem();
        auto findHyphen = deviceName.find("-");
        if (findHyphen == std::string::npos)
        {
            std::cerr << "found bad device" << deviceName << "\n";
            continue;
        }
        std::string busStr = deviceName.substr(0, findHyphen);
        std::string addrStr = deviceName.substr(findHyphen + 1);

        size_t bus = 0;
        size_t addr = 0;

        try
        {
            bus = std::stoi(busStr);
            addr = std::stoi(addrStr, 0, 16);
        }
        catch (std::invalid_argument&)
        {
            std::cerr << "Error parsing bus " << busStr << " addr " << addrStr
                      << "\n";
            continue;
        }

        const std::pair<std::string, boost::container::flat_map<
                                         std::string, BasicVariantType>>*
            baseConfig = nullptr;
        const SensorData* sensorData = nullptr;
        const std::string* interfacePath = nullptr;
        const char* sensorType = nullptr;
        size_t thresholdConfSize = 0;

        for (const std::pair<sdbusplus::message::object_path, SensorData>&
                 sensor : sensorConfigs)
        {
            sensorData = &(sensor.second);
            for (const char* type : sensorTypes)
            {
                auto sensorBase = sensorData->find(type);
                if (sensorBase != sensorData->end())
                {
                    baseConfig = &(*sensorBase);
                    sensorType = type;
                    break;
                }
            }
            if (baseConfig == nullptr)
            {
                std::cerr << "error finding base configuration for "
                          << deviceName << "\n";
                continue;
            }

            auto configBus = baseConfig->second.find("Bus");
            auto configAddress = baseConfig->second.find("Address");

            if (configBus == baseConfig->second.end() ||
                configAddress == baseConfig->second.end())
            {
                std::cerr << "error finding necessary entry in configuration\n";
                continue;
            }

            const uint64_t* confBus;
            const uint64_t* confAddr;
            if (!(confBus = std::get_if<uint64_t>(&(configBus->second))) ||
                !(confAddr = std::get_if<uint64_t>(&(configAddress->second))))
            {
                std::cerr
                    << "Cannot get bus or address, invalid configuration\n";
                continue;
            }

            if ((*confBus != bus) || (*confAddr != addr))
            {
                std::cerr << "Configuration skipping " << *confBus << "-"
                          << *confAddr << " because not " << bus << "-" << addr
                          << "\n";
                continue;
            }

            std::vector<thresholds::Threshold> confThresholds;
            if (!parseThresholdsFromConfig(*sensorData, confThresholds))
            {
                std::cerr << "error populating totoal thresholds\n";
            }
            thresholdConfSize = confThresholds.size();

            interfacePath = &(sensor.first.str);
            break;
        }
        if (interfacePath == nullptr)
        {
            // To avoid this error message, add your export map entry,
            // from Entity Manager, to sensorTypes at the top of this file.
            std::cerr << "failed to find match for " << deviceName << "\n";
            continue;
        }

        auto findPSUName = baseConfig->second.find("Name");
        if (findPSUName == baseConfig->second.end())
        {
            std::cerr << "could not determine configuration name for "
                      << deviceName << "\n";
            continue;
        }

        if (!(psuName = std::get_if<std::string>(&(findPSUName->second))))
        {
            std::cerr << "Cannot find psu name, invalid configuration\n";
            continue;
        }
        checkEvent(directory.string(), eventMatch, eventPathList);
        checkGroupEvent(directory.string(), groupEventMatch,
                        groupEventPathList);

        /* Check if there are more sensors in the same interface */
        int i = 1;
        std::vector<std::string> psuNames;
        do
        {
            // Individual string fields: Name, Name1, Name2, Name3, ...
            psuNames.push_back(std::get<std::string>(findPSUName->second));
            findPSUName = baseConfig->second.find("Name" + std::to_string(i++));
        } while (findPSUName != baseConfig->second.end());

        std::vector<fs::path> sensorPaths;
        if (!findFiles(directory, R"(\w\d+_input$)", sensorPaths, 0))
        {
            std::cerr << "No PSU non-label sensor in PSU\n";
            continue;
        }

        /* read max value in sysfs for in, curr, power, temp, ... */
        if (!findFiles(directory, R"(\w\d+_max$)", sensorPaths, 0))
        {
            if constexpr (DEBUG)
            {
                std::cerr << "No max name in PSU \n";
            }
        }

        /* Find array of labels to be exposed if it is defined in config */
        std::vector<std::string> findLabels;
        auto findLabelObj = baseConfig->second.find("Labels");
        if (findLabelObj != baseConfig->second.end())
        {
            findLabels =
                std::get<std::vector<std::string>>(findLabelObj->second);
        }

        std::regex sensorNameRegEx("([A-Za-z]+)[0-9]*_");
        std::smatch matches;

        for (const auto& sensorPath : sensorPaths)
        {
            bool maxLabel = false;
            std::string labelHead;
            std::string sensorPathStr = sensorPath.string();
            std::string sensorNameStr = sensorPath.filename();
            std::string sensorNameSubStr{""};
            if (std::regex_search(sensorNameStr, matches, sensorNameRegEx))
            {
                // hwmon *_input filename without number:
                // in, curr, power, temp, ...
                sensorNameSubStr = matches[1];
            }
            else
            {
                std::cerr << "Could not extract the alpha prefix from "
                          << sensorNameStr;
                continue;
            }

            std::string labelPath;

            /* find and differentiate _max and _input to replace "label" */
            int pos = sensorPathStr.find("_");
            if (pos != std::string::npos)
            {

                std::string sensorPathStrMax = sensorPathStr.substr(pos);
                if (sensorPathStrMax.compare("_max") == 0)
                {
                    labelPath =
                        boost::replace_all_copy(sensorPathStr, "max", "label");
                    maxLabel = true;
                }
                else
                {
                    labelPath = boost::replace_all_copy(sensorPathStr, "input",
                                                        "label");
                    maxLabel = false;
                }
            }
            else
            {
                continue;
            }

            std::ifstream labelFile(labelPath);
            if (!labelFile.good())
            {
                if constexpr (DEBUG)
                {
                    std::cerr << "Input file " << sensorPath
                              << " has no corresponding label file\n";
                }
                // hwmon *_input filename with number:
                // temp1, temp2, temp3, ...
                labelHead = sensorNameStr.substr(0, sensorNameStr.find("_"));
            }
            else
            {
                std::string label;
                std::getline(labelFile, label);
                labelFile.close();
                auto findSensor = sensors.find(label);
                if (findSensor != sensors.end())
                {
                    continue;
                }

                // hwmon corresponding *_label file contents:
                // vin1, vout1, ...
                labelHead = label.substr(0, label.find(" "));
            }

            /* append "max" for labelMatch */
            if (maxLabel)
            {
                labelHead = "max" + labelHead;
            }

            if constexpr (DEBUG)
            {
                std::cerr << "Sensor type=\"" << sensorNameSubStr
                          << "\" label=\"" << labelHead << "\"\n";
            }

            checkPWMSensor(sensorPath, labelHead, *interfacePath,
                           dbusConnection, objectServer, psuNames[0]);

            if (!findLabels.empty())
            {
                /* Check if this labelHead is enabled in config file */
                if (std::find(findLabels.begin(), findLabels.end(),
                              labelHead) == findLabels.end())
                {
                    if constexpr (DEBUG)
                    {
                        std::cerr << "could not find " << labelHead
                                  << " in the Labels list\n";
                    }
                    continue;
                }
            }

            auto findProperty = labelMatch.find(labelHead);
            if (findProperty == labelMatch.end())
            {
                if constexpr (DEBUG)
                {
                    std::cerr << "Could not find matching default property for "
                              << labelHead << "\n";
                }
                continue;
            }

            // Protect the hardcoded labelMatch list from changes,
            // by making a copy and modifying that instead.
            // Avoid bleedthrough of one device's customizations to
            // the next device, as each should be independently customizable.
            psuProperties.push_back(findProperty->second);
            auto psuProperty = psuProperties.rbegin();

            // Use label head as prefix for reading from config file,
            // example if temp1: temp1_Name, temp1_Scale, temp1_Min, ...
            std::string keyName = labelHead + "_Name";
            std::string keyScale = labelHead + "_Scale";
            std::string keyMin = labelHead + "_Min";
            std::string keyMax = labelHead + "_Max";

            bool customizedName = false;
            auto findCustomName = baseConfig->second.find(keyName);
            if (findCustomName != baseConfig->second.end())
            {
                try
                {
                    psuProperty->labelTypeName = std::visit(
                        VariantToStringVisitor(), findCustomName->second);
                }
                catch (std::invalid_argument&)
                {
                    std::cerr << "Unable to parse " << keyName << "\n";
                    continue;
                }

                // All strings are valid, including empty string
                customizedName = true;
            }

            bool customizedScale = false;
            auto findCustomScale = baseConfig->second.find(keyScale);
            if (findCustomScale != baseConfig->second.end())
            {
                try
                {
                    psuProperty->sensorScaleFactor = std::visit(
                        VariantToUnsignedIntVisitor(), findCustomScale->second);
                }
                catch (std::invalid_argument&)
                {
                    std::cerr << "Unable to parse " << keyScale << "\n";
                    continue;
                }

                // Avoid later division by zero
                if (psuProperty->sensorScaleFactor > 0)
                {
                    customizedScale = true;
                }
                else
                {
                    std::cerr << "Unable to accept " << keyScale << "\n";
                    continue;
                }
            }

            auto findCustomMin = baseConfig->second.find(keyMin);
            if (findCustomMin != baseConfig->second.end())
            {
                try
                {
                    psuProperty->minReading = std::visit(
                        VariantToDoubleVisitor(), findCustomMin->second);
                }
                catch (std::invalid_argument&)
                {
                    std::cerr << "Unable to parse " << keyMin << "\n";
                    continue;
                }
            }

            auto findCustomMax = baseConfig->second.find(keyMax);
            if (findCustomMax != baseConfig->second.end())
            {
                try
                {
                    psuProperty->maxReading = std::visit(
                        VariantToDoubleVisitor(), findCustomMax->second);
                }
                catch (std::invalid_argument&)
                {
                    std::cerr << "Unable to parse " << keyMax << "\n";
                    continue;
                }
            }

            if (!(psuProperty->minReading < psuProperty->maxReading))
            {
                std::cerr << "Min must be less than Max\n";
                continue;
            }

            // If the sensor name is being customized by config file,
            // then prefix/suffix composition becomes not necessary,
            // and in fact not wanted, because it gets in the way.
            std::string psuNameFromIndex;
            if (!customizedName)
            {
                /* Find out sensor name index for this label */
                std::regex rgx("[A-Za-z]+([0-9]+)");
                size_t nameIndex{0};
                if (std::regex_search(labelHead, matches, rgx))
                {
                    nameIndex = std::stoi(matches[1]);

                    // Decrement to preserve alignment, because hwmon
                    // human-readable filenames and labels use 1-based
                    // numbering, but the "Name", "Name1", "Name2", etc. naming
                    // convention (the psuNames vector) uses 0-based numbering.
                    if (nameIndex > 0)
                    {
                        --nameIndex;
                    }
                }
                else
                {
                    nameIndex = 0;
                }

                if (psuNames.size() <= nameIndex)
                {
                    std::cerr << "Could not pair " << labelHead
                              << " with a Name field\n";
                    continue;
                }

                psuNameFromIndex = psuNames[nameIndex];

                if constexpr (DEBUG)
                {
                    std::cerr << "Sensor label head " << labelHead
                              << " paired with " << psuNameFromIndex
                              << " at index " << nameIndex << "\n";
                }
            }

            checkEventLimits(sensorPathStr, limitEventMatch, eventPathList);

            // Similarly, if sensor scaling factor is being customized,
            // then the below power-of-10 constraint becomes unnecessary,
            // as config should be able to specify an arbitrary divisor.
            unsigned int factor = psuProperty->sensorScaleFactor;
            if (!customizedScale)
            {
                // Preserve existing usage of hardcoded labelMatch table below
                factor = std::pow(10.0, factor);

                /* Change first char of substring to uppercase */
                char firstChar = sensorNameSubStr[0] - 0x20;
                std::string strScaleFactor =
                    firstChar + sensorNameSubStr.substr(1) + "ScaleFactor";

                // Preserve existing configs by accepting earlier syntax,
                // example CurrScaleFactor, PowerScaleFactor, ...
                auto findScaleFactor = baseConfig->second.find(strScaleFactor);
                if (findScaleFactor != baseConfig->second.end())
                {
                    factor = std::visit(VariantToIntVisitor(),
                                        findScaleFactor->second);
                }

                if constexpr (DEBUG)
                {
                    std::cerr << "Sensor scaling factor " << factor
                              << " string " << strScaleFactor << "\n";
                }
            }

            std::vector<thresholds::Threshold> sensorThresholds;
            if (!parseThresholdsFromConfig(*sensorData, sensorThresholds,
                                           &labelHead))
            {
                std::cerr << "error populating thresholds for "
                          << sensorNameSubStr << "\n";
            }

            auto findSensorType = sensorTable.find(sensorNameSubStr);
            if (findSensorType == sensorTable.end())
            {
                std::cerr << sensorNameSubStr
                          << " is not a recognized sensor type\n";
                continue;
            }

            if constexpr (DEBUG)
            {
                std::cerr << "Sensor properties: Name \""
                          << psuProperty->labelTypeName << "\" Scale "
                          << psuProperty->sensorScaleFactor << " Min "
                          << psuProperty->minReading << " Max "
                          << psuProperty->maxReading << "\n";
            }

            std::string sensorName = psuProperty->labelTypeName;
            if (customizedName)
            {
                if (sensorName.empty())
                {
                    // Allow selective disabling of an individual sensor,
                    // by customizing its name to an empty string.
                    std::cerr << "Sensor disabled, empty string\n";
                    continue;
                }
            }
            else
            {
                // Sensor name not customized, do prefix/suffix composition,
                // preserving default behavior by using psuNameFromIndex.
                sensorName =
                    psuNameFromIndex + " " + psuProperty->labelTypeName;
            }

            if constexpr (DEBUG)
            {
                std::cerr << "Sensor name \"" << sensorName << "\" path \""
                          << sensorPathStr << "\" type \"" << sensorType
                          << "\"\n";
            }

            sensors[sensorName] = std::make_shared<PSUSensor>(
                sensorPathStr, sensorType, objectServer, dbusConnection, io,
                sensorName, std::move(sensorThresholds), *interfacePath,
                findSensorType->second, factor, psuProperty->maxReading,
                psuProperty->minReading, labelHead, thresholdConfSize);
            sensors[sensorName]->setupRead();
            ++numCreated;
            if constexpr (DEBUG)
            {
                std::cerr << "Created " << numCreated << " sensors so far\n";
            }
        }

        // OperationalStatus event
        combineEvents[*psuName + "OperationalStatus"] = nullptr;
        combineEvents[*psuName + "OperationalStatus"] =
            std::make_unique<PSUCombineEvent>(
                objectServer, dbusConnection, io, *psuName, eventPathList,
                groupEventPathList, "OperationalStatus");
    }

    if constexpr (DEBUG)
    {
        std::cerr << "Created total of " << numCreated << " sensors\n";
    }
    return;
}

void propertyInitialize(void)
{
    sensorTable = {{"power", "power/"},
                   {"curr", "current/"},
                   {"temp", "temperature/"},
                   {"in", "voltage/"},
                   {"fan", "fan_tach/"}};

    labelMatch = {{"pin", PSUProperty("Input Power", 3000, 0, 6)},
                  {"pout1", PSUProperty("Output Power", 3000, 0, 6)},
                  {"pout2", PSUProperty("Output Power", 3000, 0, 6)},
                  {"pout3", PSUProperty("Output Power", 3000, 0, 6)},
                  {"power1", PSUProperty("Output Power", 3000, 0, 6)},
                  {"maxpin", PSUProperty("Max Input Power", 3000, 0, 6)},
                  {"vin", PSUProperty("Input Voltage", 300, 0, 3)},
                  {"maxvin", PSUProperty("Max Input Voltage", 300, 0, 3)},
                  {"vout1", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout2", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout3", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout4", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout5", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout6", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout7", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout8", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout9", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout10", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout11", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout12", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout13", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout14", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout15", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"vout16", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"in1", PSUProperty("Output Voltage", 255, 0, 3)},
                  {"iin", PSUProperty("Input Current", 20, 0, 3)},
                  {"iout1", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout2", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout3", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout4", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout5", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout6", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout7", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout8", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout9", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout10", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout11", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout12", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout13", PSUProperty("Output Current", 255, 0, 3)},
                  {"iout14", PSUProperty("Output Current", 255, 0, 3)},
                  {"curr1", PSUProperty("Output Current", 255, 0, 3)},
                  {"maxiout1", PSUProperty("Max Output Current", 255, 0, 3)},
                  {"temp1", PSUProperty("Temperature", 127, -128, 3)},
                  {"temp2", PSUProperty("Temperature", 127, -128, 3)},
                  {"temp3", PSUProperty("Temperature", 127, -128, 3)},
                  {"temp4", PSUProperty("Temperature", 127, -128, 3)},
                  {"temp5", PSUProperty("Temperature", 127, -128, 3)},
                  {"temp6", PSUProperty("Temperature", 127, -128, 3)},
                  {"maxtemp1", PSUProperty("Max Temperature", 127, -128, 3)},
                  {"fan1", PSUProperty("Fan Speed 1", 30000, 0, 0)},
                  {"fan2", PSUProperty("Fan Speed 2", 30000, 0, 0)}};

    pwmTable = {{"fan1", "Fan_1"}, {"fan2", "Fan_2"}};

    limitEventMatch = {{"PredictiveFailure", {"max_alarm", "min_alarm"}},
                       {"Failure", {"crit_alarm", "lcrit_alarm"}}};

    eventMatch = {{"PredictiveFailure", {"power1_alarm"}},
                  {"Failure", {"in2_alarm"}},
                  {"ACLost", {"in1_beep"}},
                  {"ConfigureError", {"in1_fault"}}};

    groupEventMatch = {{"FanFault",
                        {{"fan1", {"fan1_alarm", "fan1_fault"}},
                         {"fan2", {"fan2_alarm", "fan2_fault"}}}}};
}

int main()
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);

    systemBus->request_name("xyz.openbmc_project.PSUSensor");
    sdbusplus::asio::object_server objectServer(systemBus);
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;

    propertyInitialize();

    io.post([&]() { createSensors(io, objectServer, systemBus); });
    boost::asio::deadline_timer filterTimer(io);
    std::function<void(sdbusplus::message::message&)> eventHandler =
        [&](sdbusplus::message::message& message) {
            if (message.is_method_error())
            {
                std::cerr << "callback method error\n";
                return;
            }
            filterTimer.expires_from_now(boost::posix_time::seconds(3));
            filterTimer.async_wait([&](const boost::system::error_code& ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return;
                }
                else if (ec)
                {
                    std::cerr << "timer error\n";
                }
                createSensors(io, objectServer, systemBus);
            });
        };

    for (const char* type : sensorTypes)
    {
        auto match = std::make_unique<sdbusplus::bus::match::match>(
            static_cast<sdbusplus::bus::bus&>(*systemBus),
            "type='signal',member='PropertiesChanged',path_namespace='" +
                std::string(inventoryPath) + "',arg0namespace='" + type + "'",
            eventHandler);
        matches.emplace_back(std::move(match));
    }
    io.run();
}
