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
#include "private_header.hpp"

#include "json_utils.hpp"
#include "log_id.hpp"
#include "pel_types.hpp"
#include "pel_values.hpp"

#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{

namespace pv = openpower::pels::pel_values;
using namespace phosphor::logging;

PrivateHeader::PrivateHeader(uint16_t componentID, uint32_t obmcLogID,
                             uint64_t timestamp)
{
    _header.id = static_cast<uint16_t>(SectionID::privateHeader);
    _header.size = PrivateHeader::flattenedSize();
    _header.version = privateHeaderVersion;
    _header.subType = 0;
    _header.componentID = componentID;

    _createTimestamp = getBCDTime(timestamp);

    auto now = std::chrono::system_clock::now();
    _commitTimestamp = getBCDTime(now);

    _creatorID = static_cast<uint8_t>(CreatorID::openBMC);

    // Add support for reminder and telemetry log types here if
    // ever necessary.
    _logType = 0;

    _reservedByte = 0;

    // the final section count will be updated later
    _sectionCount = 1;

    _obmcLogID = obmcLogID;

    _id = generatePELID();

    _plid = _id;

    // Leave _creatorVersion at 0

    _valid = true;
}

PrivateHeader::PrivateHeader(Stream& pel) :
    _creatorID(0), _logType(0), _reservedByte(0), _sectionCount(0),
    _obmcLogID(0), _plid(0), _id(0)
{
    try
    {
        unflatten(pel);
        validate();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Cannot unflatten private header",
                        entry("ERROR=%s", e.what()));
        _valid = false;
    }
}
std::optional<std::string> PrivateHeader::getJSON() const
{
    char tmpPhVal[50];
    sprintf(tmpPhVal, "%02X/%02X/%02X%02X %02X:%02X:%02X",
            _createTimestamp.month, _createTimestamp.day,
            _createTimestamp.yearMSB, _createTimestamp.yearLSB,
            _createTimestamp.hour, _createTimestamp.minutes,
            _createTimestamp.seconds);
    std::string phCreateTStr(tmpPhVal);
    sprintf(tmpPhVal, "%02X/%02X/%02X%02X %02X:%02X:%02X",
            _commitTimestamp.month, _commitTimestamp.day,
            _createTimestamp.yearMSB, _commitTimestamp.yearLSB,
            _commitTimestamp.hour, _commitTimestamp.minutes,
            _commitTimestamp.seconds);
    std::string phCommitTStr(tmpPhVal);
    std::string creator = getNumberString("%c", _creatorID);
    creator = pv::creatorIDs.count(creator) ? pv::creatorIDs.at(creator)
                                            : "Unknown CreatorID";
    std::string phCreatorVersionStr =
        std::string(reinterpret_cast<const char*>(_creatorVersion.version));

    std::string ph;
    jsonInsert(ph, pv::sectionVer, getNumberString("%d", privateHeaderVersion),
               1);
    jsonInsert(ph, pv::subSection, getNumberString("%d", _header.subType), 1);
    jsonInsert(ph, pv::createdBy, getNumberString("0x%X", _header.componentID),
               1);
    jsonInsert(ph, "Created at", phCreateTStr, 1);
    jsonInsert(ph, "Committed at", phCommitTStr, 1);
    jsonInsert(ph, "Creator Subsystem", creator, 1);
    jsonInsert(ph, "CSSVER", phCreatorVersionStr, 1);
    jsonInsert(ph, "Platform Log Id", getNumberString("0x%X", _plid), 1);
    jsonInsert(ph, "Entry Id", getNumberString("0x%X", _id), 1);
    jsonInsert(ph, "BMC Event Log Id", std::to_string(_obmcLogID), 1);
    ph.erase(ph.size() - 2);

    return ph;
}
void PrivateHeader::validate()
{
    bool failed = false;

    if (header().id != static_cast<uint16_t>(SectionID::privateHeader))
    {
        log<level::ERR>("Invalid private header section ID",
                        entry("ID=0x%X", header().id));
        failed = true;
    }

    if (header().version != privateHeaderVersion)
    {
        log<level::ERR>("Invalid private header version",
                        entry("VERSION=0x%X", header().version));
        failed = true;
    }

    if (_sectionCount < minSectionCount)
    {
        log<level::ERR>("Invalid section count in private header",
                        entry("SECTION_COUNT=0x%X", _sectionCount));
        failed = true;
    }

    _valid = (failed) ? false : true;
}

void PrivateHeader::unflatten(Stream& stream)
{
    stream >> _header >> _createTimestamp >> _commitTimestamp >> _creatorID >>
        _logType >> _reservedByte >> _sectionCount >> _obmcLogID >>
        _creatorVersion >> _plid >> _id;
}

void PrivateHeader::flatten(Stream& stream) const
{
    stream << _header << _createTimestamp << _commitTimestamp << _creatorID
           << _logType << _reservedByte << _sectionCount << _obmcLogID
           << _creatorVersion << _plid << _id;
}

Stream& operator>>(Stream& s, CreatorVersion& cv)
{
    for (size_t i = 0; i < sizeof(CreatorVersion); i++)
    {
        s >> cv.version[i];
    }
    return s;
}

Stream& operator<<(Stream& s, const CreatorVersion& cv)
{
    for (size_t i = 0; i < sizeof(CreatorVersion); i++)
    {
        s << cv.version[i];
    }
    return s;
}

} // namespace pels
} // namespace openpower
