#include "config_parser.hpp"

#include <algorithm>
#include <fstream>
#include <list>
#include <phosphor-logging/log.hpp>
#include <regex>
#include <string>
#include <unordered_map>

namespace phosphor
{
namespace network
{
namespace config
{

using namespace phosphor::logging;

Parser::Parser(const fs::path& filePath)
{
    setFile(filePath);
}

std::tuple<ReturnCode, KeyValueMap>
    Parser::getSection(const std::string& section)
{
    auto it = sections.find(section);
    if (it == sections.end())
    {
        KeyValueMap keyValues;
        return std::make_tuple(ReturnCode::SECTION_NOT_FOUND,
                               std::move(keyValues));
    }

    return std::make_tuple(ReturnCode::SUCCESS, it->second);
}

std::tuple<ReturnCode, ValueList> Parser::getValues(const std::string& section,
                                                    const std::string& key)
{
    ValueList values;
    KeyValueMap keyValues{};
    auto rc = ReturnCode::SUCCESS;

    std::tie(rc, keyValues) = getSection(section);
    if (rc != ReturnCode::SUCCESS)
    {
        return std::make_tuple(rc, std::move(values));
    }

    auto it = keyValues.find(key);
    if (it == keyValues.end())
    {
        return std::make_tuple(ReturnCode::KEY_NOT_FOUND, std::move(values));
    }

    for (; it != keyValues.end() && key == it->first; it++)
    {
        values.push_back(it->second);
    }

    return std::make_tuple(ReturnCode::SUCCESS, std::move(values));
}

bool Parser::isValueExist(const std::string& section, const std::string& key,
                          const std::string& value)
{
    auto rc = ReturnCode::SUCCESS;
    ValueList values;
    std::tie(rc, values) = getValues(section, key);

    if (rc != ReturnCode::SUCCESS)
    {
        return false;
    }
    auto it = std::find(values.begin(), values.end(), value);
    return it != std::end(values) ? true : false;
}

void Parser::setValue(const std::string& section, const std::string& key,
                      const std::string& value)
{
    KeyValueMap values;
    auto it = sections.find(section);
    if (it != sections.end())
    {
        values = std::move(it->second);
    }
    values.insert(std::make_pair(key, value));

    if (it != sections.end())
    {
        it->second = std::move(values);
    }
    else
    {
        sections.insert(std::make_pair(section, std::move(values)));
    }
}

#if 0
void Parser::print()
{
    for (auto section : sections)
    {
        std::cout << "[" << section.first << "]\n\n";
        for (auto keyValue : section.second)
        {
            std::cout << keyValue.first << "=" << keyValue.second << "\n";
        }
    }
}
#endif

void Parser::setFile(const fs::path& filePath)
{
    this->filePath = filePath;
    std::fstream stream;
    stream.open(filePath.string(), std::fstream::in);

    if (!stream.is_open())
    {
        return;
    }
    // clear all the section data.
    sections.clear();
    parse(stream);
    stream.close();
}

void Parser::parse(std::istream& in)
{
    static const std::regex commentRegex{R"x(\s*[;#])x"};
    static const std::regex sectionRegex{R"x(\s*\[([^\]]+)\])x"};
    static const std::regex valueRegex{R"x(\s*(\S[^ \t=]*)\s*=\s*(\S+)\s*$)x"};
    std::string section;
    std::smatch pieces;
    for (std::string line; std::getline(in, line);)
    {
        if (line.empty() || std::regex_match(line, pieces, commentRegex))
        {
            // skip comment lines and blank lines
        }
        else if (std::regex_match(line, pieces, sectionRegex))
        {
            if (pieces.size() == 2)
            {
                section = pieces[1].str();
            }
        }
        else if (std::regex_match(line, pieces, valueRegex))
        {
            if (pieces.size() == 3)
            {
                setValue(section, pieces[1].str(), pieces[2].str());
            }
        }
    }
}

} // namespace config
} // namespace network
} // namespace phosphor
