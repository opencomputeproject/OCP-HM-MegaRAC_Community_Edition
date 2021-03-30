/**
 * Copyright © 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "user_data_json.hpp"

#include "json_utils.hpp"
#include "pel_types.hpp"
#include "pel_values.hpp"
#include "stream.hpp"
#include "user_data_formats.hpp"

#include <fifo_map.hpp>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sstream>

namespace openpower::pels::user_data
{
namespace pv = openpower::pels::pel_values;
using namespace phosphor::logging;

// Use fifo_map as nlohmann::json's map. We are just ignoring the 'less'
// compare.  With this map the keys are kept in FIFO order.
template <class K, class V, class dummy_compare, class A>
using fifoMap = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using fifoJSON = nlohmann::basic_json<fifoMap>;

/**
 * @brief Returns a JSON string for use by PEL::printSectionInJSON().
 *
 * The returning string will contain a JSON object, but without
 * the outer {}.  If the input JSON isn't a JSON object (dict), then
 * one will be created with the input added to a 'Data' key.
 *
 * @param[in] json - The JSON to convert to a string
 *
 * @return std::string - The JSON string
 */
std::string prettyJSON(uint16_t componentID, uint8_t subType, uint8_t version,
                       const fifoJSON& json)
{
    fifoJSON output;
    output[pv::sectionVer] = std::to_string(version);
    output[pv::subSection] = std::to_string(subType);
    output[pv::createdBy] = getNumberString("0x%04X", componentID);

    if (!json.is_object())
    {
        output["Data"] = json;
    }
    else
    {
        for (const auto& [key, value] : json.items())
        {
            output[key] = value;
        }
    }

    // Let nlohmann do the pretty printing.
    std::stringstream stream;
    stream << std::setw(4) << output;

    auto jsonString = stream.str();

    // Now it looks like:
    // {
    //     "Section Version": ...
    //     ...
    // }

    // Since PEL::printSectionInJSON() will supply the outer { }s,
    // remove the existing ones.

    // Replace the { and the following newline, and the } and its
    // preceeding newline.
    jsonString.erase(0, 2);

    auto pos = jsonString.find_last_of('}');
    jsonString.erase(pos - 1);

    return jsonString;
}

/**
 * @brief Return a JSON string from the passed in CBOR data.
 *
 * @param[in] componentID - The comp ID from the UserData section header
 * @param[in] subType - The subtype from the UserData section header
 * @param[in] version - The version from the UserData section header
 * @param[in] data - The CBOR data
 *
 * @return std::string - The JSON string
 */
std::string getCBORJSON(uint16_t componentID, uint8_t subType, uint8_t version,
                        const std::vector<uint8_t>& data)
{
    // The CBOR parser needs the pad bytes added to 4 byte align
    // removed.  The number of bytes added to the pad is on the
    // very end, so will remove both fields before parsing.

    // If the data vector is too short, an exception will get
    // thrown which will be handled up the call stack.

    auto cborData = data;
    uint32_t pad{};

    Stream stream{cborData};
    stream.offset(cborData.size() - 4);
    stream >> pad;

    if (cborData.size() > (pad + sizeof(pad)))
    {
        cborData.resize(data.size() - sizeof(pad) - pad);
    }

    fifoJSON json = nlohmann::json::from_cbor(cborData);

    return prettyJSON(componentID, subType, version, json);
}

/**
 * @brief Return a JSON string from the passed in text data.
 *
 * The function breaks up the input text into a vector of 60 character
 * strings and converts that into JSON.  It will convert any unprintable
 * characters to periods.
 *
 * @param[in] componentID - The comp ID from the UserData section header
 * @param[in] subType - The subtype from the UserData section header
 * @param[in] version - The version from the UserData section header
 * @param[in] data - The CBOR data
 *
 * @return std::string - The JSON string
 */
std::string getTextJSON(uint16_t componentID, uint8_t subType, uint8_t version,
                        const std::vector<uint8_t>& data)
{
    constexpr size_t maxLineLength = 60;
    std::vector<std::string> text;
    size_t startPos = 0;
    bool done = false;

    // Converts any unprintable characters to periods.
    auto validate = [](char& ch) {
        if ((ch < ' ') || (ch > '~'))
        {
            ch = '.';
        }
    };

    // Break up the data into an array of 60 character strings
    while (!done)
    {
        // 60 or less characters left
        if (startPos + maxLineLength >= data.size())
        {
            std::string line{reinterpret_cast<const char*>(&data[startPos]),
                             data.size() - startPos};
            std::for_each(line.begin(), line.end(), validate);
            text.push_back(std::move(line));
            done = true;
        }
        else
        {
            std::string line{reinterpret_cast<const char*>(&data[startPos]),
                             maxLineLength};
            std::for_each(line.begin(), line.end(), validate);
            text.push_back(std::move(line));
            startPos += maxLineLength;
        }
    }

    fifoJSON json = text;
    return prettyJSON(componentID, subType, version, json);
}

/**
 * @brief Convert to an appropriate JSON string as the data is one of
 *        the formats that we natively support.
 *
 * @param[in] componentID - The comp ID from the UserData section header
 * @param[in] subType - The subtype from the UserData section header
 * @param[in] version - The version from the UserData section header
 * @param[in] data - The data itself
 *
 * @return std::optional<std::string> - The JSON string if it could be created,
 *                                      else std::nullopt.
 */
std::optional<std::string>
    getBuiltinFormatJSON(uint16_t componentID, uint8_t subType, uint8_t version,
                         const std::vector<uint8_t>& data)
{
    switch (subType)
    {
        case static_cast<uint8_t>(UserDataFormat::json):
        {
            std::string jsonString{data.begin(), data.begin() + data.size()};

            fifoJSON json = nlohmann::json::parse(jsonString);

            return prettyJSON(componentID, subType, version, json);
        }
        case static_cast<uint8_t>(UserDataFormat::cbor):
        {
            return getCBORJSON(componentID, subType, version, data);
        }
        case static_cast<uint8_t>(UserDataFormat::text):
        {
            return getTextJSON(componentID, subType, version, data);
        }
        default:
            break;
    }
    return std::nullopt;
}

std::optional<std::string> getJSON(uint16_t componentID, uint8_t subType,
                                   uint8_t version,
                                   const std::vector<uint8_t>& data)
{
    try
    {
        switch (componentID)
        {
            case static_cast<uint16_t>(ComponentID::phosphorLogging):
                return getBuiltinFormatJSON(componentID, subType, version,
                                            data);
            default:
                break;
        }
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Failed parsing UserData", entry("ERROR=%s", e.what()),
                        entry("COMP_ID=0x%X", componentID),
                        entry("SUBTYPE=0x%X", subType),
                        entry("VERSION=%d", version),
                        entry("DATA_LENGTH=%lu\n", data.size()));
    }

    return std::nullopt;
}

} // namespace openpower::pels::user_data
