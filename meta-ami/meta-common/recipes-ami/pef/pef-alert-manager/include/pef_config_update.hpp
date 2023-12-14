#pragma once
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

static bool updateJsonFile(const nlohmann::json& pefConfiguration)
{
    std::ofstream pefConfFile;
    pefConfFile.open(pefConfigFile, std::ios::trunc | std::ios::out);
    if (!pefConfFile)
    {
        std::cerr << "Failed to create file\n";
        return false;
    }
    pefConfFile << pefConfiguration.dump(4);
    pefConfFile.close();
    return true;
}

static int findEntryNo(std::string path)
{
    std::string entry;
    std::size_t found = path.find_last_of("/\\");
    entry = path.substr(found + 1);
    int i;
    for (i = 0; i < entry.length(); i++)
    {
        if (isdigit(entry[i]))
            break;
    }
    entry = entry.substr(i, entry.length() - i);
    int entryNo = atoi(entry.c_str());
    return entryNo;
}

Json parseJsonData(const std::string& configFile)
{
    std::ifstream jsonFile(configFile);
    if (!jsonFile.is_open())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "parseJSONConfig: Cannot open PEF config path");
    }
    auto data = Json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "parseJSONConfig: readings JSON parser failure");
    }

    return data;
}

static sdbusplus::bus::match::match startEventFilterTableMonitor(
    std::shared_ptr<sdbusplus::asio::connection> conn)
{
    auto EventFilterEntryMatcherCallback = [conn](sdbusplus::message::message&
                                                      msg) {
        std::string pefConfIface;
        uint8_t val = 0;
        uint16_t offsetMask = 0;
        boost::container::flat_map<std::string, std::variant<uint8_t, uint16_t>>
            propertiesChanged;
        msg.read(pefConfIface, propertiesChanged);
        std::string property = propertiesChanged.begin()->first;
        if (property != "EventData1OffsetMask")
        {
            val = std::get<uint8_t>(propertiesChanged.begin()->second);
        }
        else if (property == "EventData1OffsetMask")
        {
            offsetMask = std::get<uint16_t>(propertiesChanged.begin()->second);
        }
        std::string objPath;
        objPath = msg.get_path();
        int entryVal = 0;
        entryVal = findEntryNo(objPath.c_str());
        try
        {
            Json data = parseJsonData(pefConfigFile);
            auto& eventFilterTblData = data["EventFilterTable"];
            for (auto& value : eventFilterTblData)
            {
                int eventFilterEntry = 0;
                eventFilterEntry = value["EventFilterTableEntry"];
                if (entryVal == eventFilterEntry)
                {
                    if (property == "EventData1OffsetMask")
                    {
                        value[property] = static_cast<uint16_t>(offsetMask);
                        break;
                    }
                    else
                    {
                        value[property] = static_cast<uint8_t>(val);
                        break;
                    }
                }
            }
            Json dat = eventFilterTblData;
            dat.merge_patch(data);
            updateJsonFile(dat);
        }
        catch (nlohmann::json::exception& e)
        {
            std::cerr << "Error parsing config file";
            return;
        }
        catch (std::out_of_range& e)
        {
            std::cerr << "Error invalid type";
            return;
        }
    };
    sdbusplus::bus::match::match EventFilterEntryMatcher(
        static_cast<sdbusplus::bus::bus&>(*conn),
        "type='signal',interface='org.freedesktop.DBus.Properties',member='"
        "PropertiesChanged',arg0namespace='xyz.openbmc_project.pef."
        "EventFilterTable'",
        std::move(EventFilterEntryMatcherCallback));
    return EventFilterEntryMatcher;
}

static sdbusplus::bus::match::match startAlertPolicyTableMonitor(
    std::shared_ptr<sdbusplus::asio::connection> conn)
{
    auto AlertPolicyEntryMatcherCallback = [conn](sdbusplus::message::message&
                                                      msg) {
        std::string pefConfIface;
        uint8_t val = 0;
        boost::container::flat_map<std::string, std::variant<uint8_t, uint16_t>>
            propertiesChanged;
        msg.read(pefConfIface, propertiesChanged);
        std::string property = propertiesChanged.begin()->first;
        val = std::get<uint8_t>(propertiesChanged.begin()->second);
        std::string objPath;
        objPath = msg.get_path();
        int entryVal = 0;
        entryVal = findEntryNo(objPath.c_str());
        try
        {
            Json data = parseJsonData(pefConfigFile);
            auto& alertPolicyTblData = data["AlertPolicyTable"];
            for (auto& value : alertPolicyTblData)
            {
                int alertPolicyEntry = 0;
                alertPolicyEntry = value["AlertPolicyTableEntry"];
                if (entryVal == alertPolicyEntry)
                {
                    value[property] = static_cast<uint8_t>(val);
                    break;
                }
            }
            Json dat = alertPolicyTblData;
            dat.merge_patch(data);
            updateJsonFile(dat);
        }
        catch (nlohmann::json::exception& e)
        {
            std::cerr << "Error parsing config file";
            return;
        }
        catch (std::out_of_range& e)
        {
            std::cerr << "Error invalid type";
            return;
        }
    };
    sdbusplus::bus::match::match AlertPolicyEntryMatcher(
        static_cast<sdbusplus::bus::bus&>(*conn),
        "type='signal',interface='org.freedesktop.DBus.Properties',member='"
        "PropertiesChanged',arg0namespace='xyz.openbmc_project.pef."
        "AlertPolicyTable'",
        std::move(AlertPolicyEntryMatcherCallback));
    return AlertPolicyEntryMatcher;
}

static sdbusplus::bus::match::match
    startPefConfInfoMonitor(std::shared_ptr<sdbusplus::asio::connection> conn)
{
    auto PefConfInfoMatcherCallback = [conn](sdbusplus::message::message& msg) {
        std::string pefConfIface;
        uint8_t val = 0;
        std::vector<std::string> rec;
        std::string subject;
        std::string message;
        uint16_t selId = 0;
        boost::container::flat_map<std::string,
                                   std::variant<uint8_t, uint16_t, std::string,
                                                std::vector<std::string>>>
            propertiesChanged;
        msg.read(pefConfIface, propertiesChanged);
        std::string property = propertiesChanged.begin()->first;
        if ((property == "LastSWProcessedEventID") ||
            (property == "LastBMCProcessedEventID"))
        {
            selId = std::get<uint16_t>(propertiesChanged.begin()->second);
        }
        else if (property == "Recipient")
        {
            rec = std::get<std::vector<std::string>>(
                propertiesChanged.begin()->second);
        }

        else if (property == "Subject")
        {
            subject = std::get<std::string>(propertiesChanged.begin()->second);
        }
        else if (property == "Message")
        {
            message = std::get<std::string>(propertiesChanged.begin()->second);
        }

        else
        {
            val = std::get<uint8_t>(propertiesChanged.begin()->second);
        }
        std::string objPath;
        objPath = msg.get_path();
        try
        {
            Json data = parseJsonData(pefConfigFile);
            auto& pefConfData = data["PEFConfInfo"];
            for (auto& value : pefConfData)
            {
                if ((property == "LastSWProcessedEventID") ||
                    (property == "LastBMCProcessedEventID"))
                {
                    value[property] = static_cast<uint16_t>(selId);
                }
                else if (property == "Recipient")
                {
                    value[property] =
                        static_cast<std::vector<std::string>>(rec);
                }

                else if (property == "Subject")
                {
                    value[property] = static_cast<std::string>(subject);
                }
                else if (property == "Message")
                {
                    value[property] = static_cast<std::string>(message);
                }

                else
                {
                    value[property] = static_cast<uint8_t>(val);
                }
            }
            Json data2 = pefConfData;
            data2.merge_patch(data);
            updateJsonFile(data2);
        }
        catch (nlohmann::json::exception& e)
        {
            std::cerr << "Error parsing config file";
            return;
        }
        catch (std::out_of_range& e)
        {
            std::cerr << "Error invalid type";
            return;
        }
    };
    sdbusplus::bus::match::match PefConfInfoEntryMatcher(
        static_cast<sdbusplus::bus::bus&>(*conn),
        "type='signal',interface='org.freedesktop.DBus.Properties',member='"
        "PropertiesChanged',arg0namespace='xyz.openbmc_project.pef."
        "PEFConfInfo'",
        std::move(PefConfInfoMatcherCallback));
    return PefConfInfoEntryMatcher;
}
