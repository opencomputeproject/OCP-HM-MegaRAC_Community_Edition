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

#pragma once
#include <ipmi_to_redfish_hooks.hpp>

namespace intel_oem::ipmi::sel::redfish_hooks::me
{
enum class EventSensor
{
    MeFirmwareHealth = 23
};

enum class HealthEventType
{
    FirmwareStatus = 0x00,
    SmbusLinkFailure = 0x01
};

bool messageHook(const SELData& selData, const std::string& ipmiRaw);

namespace utils
{
// Maps event byte to human-readable message
using MessageMap = boost::container::flat_map<uint8_t, std::string>;
// Function which performs custom decoding for complex structures
using ParserFunc = std::function<bool(const SELData& selData, std::string&,
                                      std::vector<std::string>&)>;
/**
 * @brief Generic function for parsing IPMI Platform Events
 *
 * @param[in] map - maps EventData2 byte of IPMI Platform Event to decoder
 * @param[in] selData - IPMI Platform Event
 * @param[out] eventId - resulting Redfish Event ID
 * @param[out] args - resulting Redfish Event Parameters
 *
 * @returns If matching event was found
 */
static inline bool genericMessageHook(
    const boost::container::flat_map<
        uint8_t,
        std::pair<std::string,
                  std::optional<std::variant<ParserFunc, MessageMap>>>>& map,
    const SELData& selData, std::string& eventId,
    std::vector<std::string>& args)
{
    const auto match = map.find(selData.eventData2);
    if (match == map.end())
    {
        return false;
    }

    eventId = match->second.first;

    auto details = match->second.second;
    if (details)
    {
        if (std::holds_alternative<MessageMap>(*details))
        {
            const auto& detailsMap = std::get<MessageMap>(*details);
            const auto translation = detailsMap.find(selData.eventData3);
            if (translation == detailsMap.end())
            {
                return false;
            }

            args.push_back(translation->second);
        }
        else if (std::holds_alternative<ParserFunc>(*details))
        {
            const auto& parser = std::get<ParserFunc>(*details);
            return parser(selData, eventId, args);
        }
        else
        {
            return false;
        }
    }

    return true;
}

static inline std::string toHex(uint8_t byte)
{
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << std::setfill('0')
       << std::setw(2) << static_cast<int>(byte);
    return ss.str();
}

template <int idx>
static inline bool
    logByte(const SELData& selData, std::string& unused,
            std::vector<std::string>& args,
            std::function<std::string(uint8_t)> conversion = nullptr)
{
    uint8_t byte;
    switch (idx)
    {
        case 0:
            byte = selData.offset;
            break;

        case 1:
            byte = selData.eventData2;
            break;

        case 2:
            byte = selData.eventData3;
            break;

        default:
            return false;
            break;
    }

    if (conversion)
    {
        args.push_back(conversion(byte));
    }
    else
    {
        args.push_back(std::to_string(byte));
    }

    return true;
}

template <int idx>
static inline bool logByteDec(const SELData& selData, std::string& unused,
                              std::vector<std::string>& args)
{
    return logByte<idx>(selData, unused, args);
}

template <int idx>
static inline bool logByteHex(const SELData& selData, std::string& unused,
                              std::vector<std::string>& args)
{
    return logByte<idx>(selData, unused, args, toHex);
}

static inline void storeRedfishEvent(const std::string& ipmiRaw,
                                     const std::string& eventId,
                                     const std::vector<std::string>& args)
{
    static constexpr std::string_view openBMCMessageRegistryVer = "0.1";
    std::string messageID =
        "OpenBMC." + std::string(openBMCMessageRegistryVer) + "." + eventId;

    // Log the Redfish message to the journal with the appropriate metadata
    std::string journalMsg = "ME Event: " + ipmiRaw;
    if (args.empty())
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            journalMsg.c_str(),
            phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                     messageID.c_str()));
    }
    else
    {
        std::string argsStr = boost::algorithm::join(args, ",");
        phosphor::logging::log<phosphor::logging::level::INFO>(
            journalMsg.c_str(),
            phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                     messageID.c_str()),
            phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s",
                                     argsStr.c_str()));
    }
}
} // namespace utils
} // namespace intel_oem::ipmi::sel::redfish_hooks::me