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
#include "ascii_string.hpp"

#include "pel_types.hpp"

#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{
namespace src
{

using namespace phosphor::logging;

AsciiString::AsciiString(Stream& stream)
{
    unflatten(stream);
}

AsciiString::AsciiString(const message::Entry& entry)
{
    // Power Error:  1100RRRR
    // BMC Error:    BDSSRRRR
    // where:
    //  RRRR = reason code
    //  SS = subsystem ID

    // First is type, like 'BD'
    setByte(0, entry.src.type);

    // Next is '00', or subsystem ID
    if (entry.src.type == static_cast<uint8_t>(SRCType::powerError))
    {
        setByte(2, 0x00);
    }
    else // BMC Error
    {
        setByte(2, entry.subsystem);
    }

    // Then the reason code
    setByte(4, entry.src.reasonCode >> 8);
    setByte(6, entry.src.reasonCode & 0xFF);

    // Padded with spaces
    for (size_t offset = 8; offset < asciiStringSize; offset++)
    {
        _string[offset] = ' ';
    }
}

void AsciiString::flatten(Stream& stream) const
{
    stream.write(_string.data(), _string.size());
}

void AsciiString::unflatten(Stream& stream)
{
    stream.read(_string.data(), _string.size());

    // Only allow certain ASCII characters as other entities will
    // eventually want to display this.
    std::for_each(_string.begin(), _string.end(), [](auto& c) {
        if (!isalnum(c) && (c != ' ') && (c != '.') && (c != ':') && (c != '/'))
        {
            c = ' ';
        }
    });
}

std::string AsciiString::get() const
{
    std::string string{_string.begin(), _string.begin() + _string.size()};
    return string;
}

void AsciiString::setByte(size_t byteOffset, uint8_t value)
{
    assert(byteOffset < asciiStringSize);

    char characters[3];
    sprintf(characters, "%02X", value);

    auto writeOffset = byteOffset;
    _string[writeOffset++] = characters[0];
    _string[writeOffset] = characters[1];
}

} // namespace src
} // namespace pels
} // namespace openpower
