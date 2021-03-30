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
#include "propertywatchimpl.hpp"

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/** @brief convert index.
 *
 *  An optimal start implementation requires objects organized in the
 *  same structure as the mapper response.  The convert method reorganizes
 *  the flat structure of the index to match.
 *
 *  @param[in] index - The index to be converted.
 */
MappedPropertyIndex convert(const PropertyIndex& index)
{
    MappedPropertyIndex m;

    for (const auto& i : index)
    {
        const auto& path = std::get<pathIndex>(i.first);
        const auto& interface = std::get<interfaceIndex>(i.first);
        const auto& property = std::get<propertyIndex>(i.first);
        m[path][interface].push_back(property);
    }

    return m;
}
} // namespace monitoring
} // namespace dbus
} // namespace phosphor
