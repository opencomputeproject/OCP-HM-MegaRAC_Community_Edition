/**
 * Copyright © 2019 IBM Corporation
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
#include "user_data.hpp"

#include "json_utils.hpp"
#include "pel_types.hpp"
#include "user_data_formats.hpp"
#ifdef PELTOOL
#include "user_data_json.hpp"
#endif

#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{

using namespace phosphor::logging;

void UserData::unflatten(Stream& stream)
{
    stream >> _header;

    if (_header.size <= SectionHeader::flattenedSize())
    {
        throw std::out_of_range(
            "UserData::unflatten: SectionHeader::size field too small");
    }

    size_t dataLength = _header.size - SectionHeader::flattenedSize();
    _data.resize(dataLength);

    stream >> _data;
}

void UserData::flatten(Stream& stream) const
{
    stream << _header << _data;
}

UserData::UserData(Stream& pel)
{
    try
    {
        unflatten(pel);
        validate();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Cannot unflatten user data",
                        entry("ERROR=%s", e.what()));
        _valid = false;
    }
}

UserData::UserData(uint16_t componentID, uint8_t subType, uint8_t version,
                   const std::vector<uint8_t>& data)
{
    _header.id = static_cast<uint16_t>(SectionID::userData);
    _header.size = Section::flattenedSize() + data.size();
    _header.version = version;
    _header.subType = subType;
    _header.componentID = componentID;

    _data = data;

    _valid = true;
}

void UserData::validate()
{
    if (header().id != static_cast<uint16_t>(SectionID::userData))
    {
        log<level::ERR>("Invalid user data section ID",
                        entry("ID=0x%X", header().id));
        _valid = false;
    }
    else
    {
        _valid = true;
    }
}

std::optional<std::string> UserData::getJSON() const
{
#ifdef PELTOOL
    return user_data::getJSON(_header.componentID, _header.subType,
                              _header.version, _data);
#endif
    return std::nullopt;
}

bool UserData::shrink(size_t newSize)
{
    // minimum size is 4 bytes plus the 8B header
    if ((newSize < flattenedSize()) &&
        (newSize >= (Section::flattenedSize() + 4)))
    {
        auto dataSize = newSize - Section::flattenedSize();

        // Ensure it's 4B aligned
        _data.resize((dataSize / 4) * 4);
        _header.size = Section::flattenedSize() + _data.size();
        return true;
    }

    return false;
}

} // namespace pels
} // namespace openpower
