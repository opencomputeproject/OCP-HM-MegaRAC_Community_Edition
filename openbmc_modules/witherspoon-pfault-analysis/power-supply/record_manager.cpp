/**
 * Copyright © 2017 IBM Corporation
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
#include "record_manager.hpp"

#include <math.h>

#include <phosphor-logging/log.hpp>

#include <chrono>

namespace witherspoon
{
namespace power
{
namespace history
{

using namespace phosphor::logging;

bool RecordManager::add(const std::vector<uint8_t>& rawRecord)
{
    if (rawRecord.size() == 0)
    {
        // The PS has no data - either the power supply just started up,
        // or it just got a SYNC.  Clear the history.
        records.clear();
        return true;
    }

    try
    {
        // Peek at the ID to see if more processing is needed.
        auto id = getRawRecordID(rawRecord);

        if (!records.empty())
        {
            auto previousID = std::get<recIDPos>(records.front());

            // Already have this record.  Done.
            if (previousID == id)
            {
                return false;
            }

            // Check that the sequence ID is in order.
            // If not, clear out current list.
            if ((previousID + 1) != id)
            {
                // If it just rolled over from 0xFF to 0x00, then no
                // need to clear.  If we see a 0 seemingly out of nowhere,
                // then it was a sync so clear the old records.
                auto rolledOver =
                    (previousID == lastSequenceID) && (id == FIRST_SEQUENCE_ID);

                if (!rolledOver)
                {
                    if (id != FIRST_SEQUENCE_ID)
                    {
                        log<level::INFO>(
                            "Noncontiguous INPUT_HISTORY sequence ID "
                            "found. Clearing old entries",
                            entry("OLD_ID=%ld", previousID),
                            entry("NEW_ID=%ld", id));
                    }
                    records.clear();
                }
            }
        }

        records.push_front(std::move(createRecord(rawRecord)));

        // If no more should be stored, prune the oldest
        if (records.size() > maxRecords)
        {
            records.pop_back();
        }
    }
    catch (InvalidRecordException& e)
    {
        return false;
    }

    return true;
}

auto RecordManager::getAverageRecords() -> DBusRecordList
{
    DBusRecordList list;

    for (const auto& r : records)
    {
        list.emplace_back(std::get<recTimePos>(r), std::get<recAvgPos>(r));
    }

    return list;
}

auto RecordManager::getMaximumRecords() -> DBusRecordList
{
    DBusRecordList list;

    for (const auto& r : records)
    {
        list.emplace_back(std::get<recTimePos>(r), std::get<recMaxPos>(r));
    }

    return list;
}

size_t RecordManager::getRawRecordID(const std::vector<uint8_t>& data) const
{
    if (data.size() != RAW_RECORD_SIZE)
    {
        log<level::ERR>("Invalid INPUT_HISTORY size",
                        entry("SIZE=%d", data.size()));
        throw InvalidRecordException{};
    }

    return data[RAW_RECORD_ID_OFFSET];
}

Record RecordManager::createRecord(const std::vector<uint8_t>& data)
{
    // The raw record format is:
    //  0xAABBCCDDEE
    //
    //  where:
    //    0xAA = sequence ID
    //    0xBBCC = average power in linear format (0xCC = MSB)
    //    0xDDEE = maximum power in linear format (0xEE = MSB)
    auto id = getRawRecordID(data);

    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

    auto val = static_cast<uint16_t>(data[2]) << 8 | data[1];
    auto averagePower = linearToInteger(val);

    val = static_cast<uint16_t>(data[4]) << 8 | data[3];
    auto maxPower = linearToInteger(val);

    return Record{id, time, averagePower, maxPower};
}

int64_t RecordManager::linearToInteger(uint16_t data)
{
    // The exponent is the first 5 bits, followed by 11 bits of mantissa.
    int8_t exponent = (data & 0xF800) >> 11;
    int16_t mantissa = (data & 0x07FF);

    // If exponent's MSB on, then it's negative.
    // Convert from two's complement.
    if (exponent & 0x10)
    {
        exponent = (~exponent) & 0x1F;
        exponent = (exponent + 1) * -1;
    }

    // If mantissa's MSB on, then it's negative.
    // Convert from two's complement.
    if (mantissa & 0x400)
    {
        mantissa = (~mantissa) & 0x07FF;
        mantissa = (mantissa + 1) * -1;
    }

    auto value = static_cast<float>(mantissa) * pow(2, exponent);
    return value;
}

} // namespace history
} // namespace power
} // namespace witherspoon
