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
#include "mru.hpp"

#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{
namespace src
{

using namespace phosphor::logging;

MRU::MRU(Stream& pel)
{
    pel >> _type >> _size >> _flags >> _reserved4B;

    size_t numMRUs = _flags & 0xF;

    for (size_t i = 0; i < numMRUs; i++)
    {
        MRUCallout mru;
        pel >> mru.priority;
        pel >> mru.id;
        _mrus.push_back(std::move(mru));
    }

    size_t actualSize = sizeof(_type) + sizeof(_size) + sizeof(_flags) +
                        sizeof(_reserved4B) +
                        (sizeof(MRUCallout) * _mrus.size());
    if (_size != actualSize)
    {
        log<level::WARNING>("MRU callout section in PEL has listed size that "
                            "doesn't match actual size",
                            entry("SUBSTRUCTURE_SIZE=%lu", _size),
                            entry("NUM_MRUS=%lu", _mrus.size()),
                            entry("ACTUAL_SIZE=%lu", actualSize));
    }
}

void MRU::flatten(Stream& pel) const
{
    pel << _type << _size << _flags << _reserved4B;

    for (auto& mru : _mrus)
    {
        pel << mru.priority;
        pel << mru.id;
    }
}
} // namespace src
} // namespace pels
} // namespace openpower
