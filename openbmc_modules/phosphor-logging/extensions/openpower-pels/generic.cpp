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
#include "generic.hpp"

#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{

using namespace phosphor::logging;

void Generic::unflatten(Stream& stream)
{
    stream >> _header;

    if (_header.size <= SectionHeader::flattenedSize())
    {
        throw std::out_of_range(
            "Generic::unflatten: SectionHeader::size field too small");
    }

    size_t dataLength = _header.size - SectionHeader::flattenedSize();
    _data.resize(dataLength);

    stream >> _data;
}

void Generic::flatten(Stream& stream) const
{
    stream << _header << _data;
}

Generic::Generic(Stream& pel)
{
    try
    {
        unflatten(pel);
        validate();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Cannot unflatten generic section",
                        entry("ERROR=%s", e.what()));
        _valid = false;
    }
}

void Generic::validate()
{
    // Nothing to validate
    _valid = true;
}

} // namespace pels
} // namespace openpower
