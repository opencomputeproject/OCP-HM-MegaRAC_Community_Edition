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
#include <systemd/sd-journal.h>

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <pulse_event_monitor.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sel_logger.hpp>
#include <threshold_event_monitor.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

struct DBusInternalError final : public sdbusplus::exception_t
{
    const char* name() const noexcept override
    {
        return "org.freedesktop.DBus.Error.Failed";
    };
    const char* description() const noexcept override
    {
        return "internal error";
    };
    const char* what() const noexcept override
    {
        return "org.freedesktop.DBus.Error.Failed: "
               "internal error";
    };
};

static bool getSELLogFiles(std::vector<std::filesystem::path>& selLogFiles)
{
    // Loop through the directory looking for ipmi_sel log files
    for (const std::filesystem::directory_entry& dirEnt :
         std::filesystem::directory_iterator(selLogDir))
    {
        std::string filename = dirEnt.path().filename();
        if (boost::starts_with(filename, selLogFilename))
        {
            // If we find an ipmi_sel log file, save the path
            selLogFiles.emplace_back(selLogDir / filename);
        }
    }
    // As the log files rotate, they are appended with a ".#" that is higher for
    // the older logs. Since we don't expect more than 10 log files, we
    // can just sort the list to get them in order from newest to oldest
    std::sort(selLogFiles.begin(), selLogFiles.end());

    return !selLogFiles.empty();
}

static unsigned int initializeRecordId(void)
{
    std::vector<std::filesystem::path> selLogFiles;
    if (!getSELLogFiles(selLogFiles))
    {
        return selInvalidRecID;
    }
    std::ifstream logStream(selLogFiles.front());
    if (!logStream.is_open())
    {
        return selInvalidRecID;
    }
    std::string line;
    std::string newestEntry;
    while (std::getline(logStream, line))
    {
        newestEntry = line;
    }

    std::vector<std::string> newestEntryFields;
    boost::split(newestEntryFields, newestEntry, boost::is_any_of(" ,"),
                 boost::token_compress_on);
    if (newestEntryFields.size() < 4)
    {
        return selInvalidRecID;
    }

    return std::stoul(newestEntryFields[1]);
}

static unsigned int getNewRecordId(void)
{
    static unsigned int recordId = initializeRecordId();

    // If the log has been cleared, also clear the current ID
    std::vector<std::filesystem::path> selLogFiles;
    if (!getSELLogFiles(selLogFiles))
    {
        recordId = selInvalidRecID;
    }

    if (++recordId >= selInvalidRecID)
    {
        recordId = 1;
    }
    return recordId;
}

static void toHexStr(const std::vector<uint8_t>& data, std::string& hexStr)
{
    std::stringstream stream;
    stream << std::hex << std::uppercase << std::setfill('0');
    for (const int& v : data)
    {
        stream << std::setw(2) << v;
    }
    hexStr = stream.str();
}

template <typename... T>
static uint16_t
    selAddSystemRecord(const std::string& message, const std::string& path,
                       const std::vector<uint8_t>& selData, const bool& assert,
                       const uint16_t& genId, T&&... metadata)
{
    // Only 3 bytes of SEL event data are allowed in a system record
    if (selData.size() > selEvtDataMaxSize)
    {
        throw std::invalid_argument("Event data too large");
    }
    std::string selDataStr;
    toHexStr(selData, selDataStr);

    unsigned int recordId = getNewRecordId();
    sd_journal_send("MESSAGE=%s", message.c_str(), "PRIORITY=%i", selPriority,
                    "MESSAGE_ID=%s", selMessageId, "IPMI_SEL_RECORD_ID=%d",
                    recordId, "IPMI_SEL_RECORD_TYPE=%x", selSystemType,
                    "IPMI_SEL_GENERATOR_ID=%x", genId,
                    "IPMI_SEL_SENSOR_PATH=%s", path.c_str(),
                    "IPMI_SEL_EVENT_DIR=%x", assert, "IPMI_SEL_DATA=%s",
                    selDataStr.c_str(), std::forward<T>(metadata)..., NULL);
    return recordId;
}

static uint16_t selAddOemRecord(const std::string& message,
                                const std::vector<uint8_t>& selData,
                                const uint8_t& recordType)
{
    // A maximum of 13 bytes of SEL event data are allowed in an OEM record
    if (selData.size() > selOemDataMaxSize)
    {
        throw std::invalid_argument("Event data too large");
    }
    std::string selDataStr;
    toHexStr(selData, selDataStr);

    unsigned int recordId = getNewRecordId();
    sd_journal_send("MESSAGE=%s", message.c_str(), "PRIORITY=%i", selPriority,
                    "MESSAGE_ID=%s", selMessageId, "IPMI_SEL_RECORD_ID=%d",
                    recordId, "IPMI_SEL_RECORD_TYPE=%x", recordType,
                    "IPMI_SEL_DATA=%s", selDataStr.c_str(), NULL);
    return recordId;
}

int main(int argc, char* argv[])
{
    // setup connection to dbus
    boost::asio::io_service io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // IPMI SEL Object
    conn->request_name(ipmiSelObject);
    auto server = sdbusplus::asio::object_server(conn);

    // Add SEL Interface
    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceAddSel =
        server.add_interface(ipmiSelPath, ipmiSelAddInterface);

    // Add a new SEL entry
    ifaceAddSel->register_method(
        "IpmiSelAdd", [](const std::string& message, const std::string& path,
                         const std::vector<uint8_t>& selData,
                         const bool& assert, const uint16_t& genId) {
            return selAddSystemRecord(message, path, selData, assert, genId);
        });
    // Add a new OEM SEL entry
    ifaceAddSel->register_method(
        "IpmiSelAddOem",
        [](const std::string& message, const std::vector<uint8_t>& selData,
           const uint8_t& recordType) {
            return selAddOemRecord(message, selData, recordType);
        });
    ifaceAddSel->initialize();

#ifdef SEL_LOGGER_MONITOR_THRESHOLD_EVENTS
    sdbusplus::bus::match::match thresholdAssertMonitor =
        startThresholdAssertMonitor(conn);
#endif

#ifdef REDFISH_LOG_MONITOR_PULSE_EVENTS
    sdbusplus::bus::match::match pulseEventMonitor =
        startPulseEventMonitor(conn);
#endif

    io.run();

    return 0;
}
