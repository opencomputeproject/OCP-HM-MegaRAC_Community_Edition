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
#include "pce_identity.hpp"

namespace openpower
{
namespace pels
{
namespace src
{

PCEIdentity::PCEIdentity(Stream& pel)
{
    pel >> _type >> _size >> _flags >> _mtms;

    // Whatever is left is the enclosure name.
    if (_size < (4 + _mtms.flattenedSize()))
    {
        throw std::runtime_error("PCE identity structure size field too small");
    }

    size_t pceNameSize = _size - (4 + _mtms.flattenedSize());

    _pceName.resize(pceNameSize);
    pel >> _pceName;
}

void PCEIdentity::flatten(Stream& pel) const
{
    pel << _type << _size << _flags << _mtms << _pceName;
}

} // namespace src
} // namespace pels
} // namespace openpower
