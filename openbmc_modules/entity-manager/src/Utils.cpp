/*
// Copyright (c) 2017 Intel Corporation
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

#include "Utils.hpp"

#include "VariantVisitors.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/lexical_cast.hpp>
#include <sdbusplus/bus/match.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

#include <filesystem>
#include <fstream>
#include <regex>

constexpr const char* templateChar = "$";

namespace fs = std::filesystem;
static bool powerStatusOn = false;
static std::unique_ptr<sdbusplus::bus::match::match> powerMatch = nullptr;

bool findFiles(const fs::path& dirPath, const std::string& matchString,
               std::vector<fs::path>& foundPaths)
{
    if (!fs::exists(dirPath))
        return false;

    std::regex search(matchString);
    std::smatch match;
    for (const auto& p : fs::directory_iterator(dirPath))
    {
        std::string path = p.path().string();
        if (std::regex_search(path, match, search))
        {
            foundPaths.emplace_back(p.path());
        }
    }
    return true;
}

bool getI2cDevicePaths(const fs::path& dirPath,
                       boost::container::flat_map<size_t, fs::path>& busPaths)
{
    if (!fs::exists(dirPath))
    {
        return false;
    }

    // Regex for matching the path
    std::regex searchPath(std::string(R"(i2c-\d+$)"));
    // Regex for matching the bus numbers
    std::regex searchBus(std::string(R"(\w[^-]*$)"));
    std::smatch matchPath;
    std::smatch matchBus;
    for (const auto& p : fs::directory_iterator(dirPath))
    {
        std::string path = p.path().string();
        if (std::regex_search(path, matchPath, searchPath))
        {
            if (std::regex_search(path, matchBus, searchBus))
            {
                size_t bus = stoul(*matchBus.begin());
                busPaths.insert(std::pair<size_t, fs::path>(bus, p.path()));
            }
        }
    }

    return true;
}

bool validateJson(const nlohmann::json& schemaFile, const nlohmann::json& input)
{
    valijson::Schema schema;
    valijson::SchemaParser parser;
    valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaFile);
    parser.populateSchema(schemaAdapter, schema);
    valijson::Validator validator;
    valijson::adapters::NlohmannJsonAdapter targetAdapter(input);
    if (!validator.validate(schema, targetAdapter, NULL))
    {
        return false;
    }
    return true;
}

bool isPowerOn(void)
{
    if (!powerMatch)
    {
        throw std::runtime_error("Power Match Not Created");
    }
    return powerStatusOn;
}

void setupPowerMatch(const std::shared_ptr<sdbusplus::asio::connection>& conn)
{
    powerMatch = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*conn),
        "type='signal',interface='" + std::string(properties::interface) +
            "',path='" + std::string(power::path) + "',arg0='" +
            std::string(power::interface) + "'",
        [](sdbusplus::message::message& message) {
            std::string objectName;
            boost::container::flat_map<std::string, std::variant<std::string>>
                values;
            message.read(objectName, values);
            auto findState = values.find(power::property);
            if (findState != values.end())
            {
                powerStatusOn = boost::ends_with(
                    std::get<std::string>(findState->second), "Running");
            }
        });

    conn->async_method_call(
        [](boost::system::error_code ec,
           const std::variant<std::string>& state) {
            if (ec)
            {
                return;
            }
            powerStatusOn =
                boost::ends_with(std::get<std::string>(state), "Running");
        },
        power::busname, power::path, properties::interface, properties::get,
        power::interface, power::property);
}

// finds the template character (currently set to $) and replaces the value with
// the field found in a dbus object i.e. $ADDRESS would get populated with the
// ADDRESS field from a object on dbus
std::optional<std::string> templateCharReplace(
    nlohmann::json::iterator& keyPair,
    const boost::container::flat_map<std::string, BasicVariantType>&
        foundDevice,
    const size_t foundDeviceIdx, const std::optional<std::string>& replaceStr)
{
    std::optional<std::string> ret = std::nullopt;

    if (keyPair.value().type() == nlohmann::json::value_t::object ||
        keyPair.value().type() == nlohmann::json::value_t::array)
    {
        for (auto nextLayer = keyPair.value().begin();
             nextLayer != keyPair.value().end(); nextLayer++)
        {
            templateCharReplace(nextLayer, foundDevice, foundDeviceIdx,
                                replaceStr);
        }
        return ret;
    }

    std::string* strPtr = keyPair.value().get_ptr<std::string*>();
    if (strPtr == nullptr)
    {
        return ret;
    }

    boost::replace_all(*strPtr, std::string(templateChar) + "index",
                       std::to_string(foundDeviceIdx));
    if (replaceStr)
    {
        boost::replace_all(*strPtr, *replaceStr,
                           std::to_string(foundDeviceIdx));
    }

    for (auto& foundDevicePair : foundDevice)
    {
        std::string templateName = templateChar + foundDevicePair.first;
        boost::iterator_range<std::string::const_iterator> find =
            boost::ifind_first(*strPtr, templateName);
        if (find)
        {
            constexpr const std::array<char, 5> mathChars = {'+', '-', '%', '*',
                                                             '/'};
            size_t start = find.begin() - strPtr->begin();
            size_t nextItemIdx = start + templateName.size() + 1;

            // check for additional operations
            if (!start && find.end() == strPtr->end())
            {
                std::visit([&](auto&& val) { keyPair.value() = val; },
                           foundDevicePair.second);
                return ret;
            }
            else if (nextItemIdx > strPtr->size() ||
                     std::find(mathChars.begin(), mathChars.end(),
                               strPtr->at(nextItemIdx)) == mathChars.end())
            {
                std::string val = std::visit(VariantToStringVisitor(),
                                             foundDevicePair.second);
                boost::ireplace_all(*strPtr,
                                    templateChar + foundDevicePair.first, val);
                continue;
            }

            // save the prefix
            std::string prefix = strPtr->substr(0, start);

            // operate on the rest
            std::string end = strPtr->substr(nextItemIdx);

            std::vector<std::string> split;
            boost::split(split, end, boost::is_any_of(" "));

            // need at least 1 operation and number
            if (split.size() < 2)
            {
                std::cerr << "Syntax error on template replacement of "
                          << *strPtr << "\n";
                for (const std::string& data : split)
                {
                    std::cerr << data << " ";
                }
                std::cerr << "\n";
                continue;
            }

            // we assume that the replacement is a number, because we can
            // only do math on numbers.. we might concatenate strings in the
            // future, but thats later
            int number =
                std::visit(VariantToIntVisitor(), foundDevicePair.second);

            bool isOperator = true;
            TemplateOperation next = TemplateOperation::addition;

            auto it = split.begin();

            for (; it != split.end(); it++)
            {
                if (isOperator)
                {
                    if (*it == "+")
                    {
                        next = TemplateOperation::addition;
                    }
                    else if (*it == "-")
                    {
                        next = TemplateOperation::subtraction;
                    }
                    else if (*it == "*")
                    {
                        next = TemplateOperation::multiplication;
                    }
                    else if (*it == R"(%)")
                    {
                        next = TemplateOperation::modulo;
                    }
                    else if (*it == R"(/)")
                    {
                        next = TemplateOperation::division;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    int constant = 0;
                    try
                    {
                        constant = std::stoi(*it);
                    }
                    catch (std::invalid_argument&)
                    {
                        std::cerr << "Parameter not supported for templates "
                                  << *it << "\n";
                        continue;
                    }
                    switch (next)
                    {
                        case TemplateOperation::addition:
                        {
                            number += constant;
                            break;
                        }
                        case TemplateOperation::subtraction:
                        {
                            number -= constant;
                            break;
                        }
                        case TemplateOperation::multiplication:
                        {
                            number *= constant;
                            break;
                        }
                        case TemplateOperation::division:
                        {
                            number /= constant;
                            break;
                        }
                        case TemplateOperation::modulo:
                        {
                            number = number % constant;
                            break;
                        }

                        default:
                            break;
                    }
                }
                isOperator = !isOperator;
            }

            std::string result = prefix + std::to_string(number);

            std::string replaced(find.begin(), find.end());
            for (auto it2 = split.begin(); it2 != split.end(); it2++)
            {
                replaced += " ";
                replaced += *it2;
                if (it2 == it)
                {
                    break;
                }
            }
            ret = replaced;

            if (it != split.end())
            {
                for (; it != split.end(); it++)
                {
                    result += " " + *it;
                }
            }
            keyPair.value() = result;

            // We probably just invalidated the pointer above, so set it to null
            strPtr = nullptr;
            break;
        }
    }

    strPtr = keyPair.value().get_ptr<std::string*>();
    if (strPtr == nullptr)
    {
        return ret;
    }

    // convert hex numbers to ints
    if (boost::starts_with(*strPtr, "0x"))
    {
        try
        {
            size_t pos = 0;
            int64_t temp = std::stoul(*strPtr, &pos, 0);
            if (pos == strPtr->size())
            {
                keyPair.value() = static_cast<uint64_t>(temp);
            }
        }
        catch (std::invalid_argument&)
        {}
        catch (std::out_of_range&)
        {}
    }
    // non-hex numbers
    else
    {
        try
        {
            uint64_t temp = boost::lexical_cast<uint64_t>(*strPtr);
            keyPair.value() = temp;
        }
        catch (boost::bad_lexical_cast&)
        {}
    }
    return ret;
}
