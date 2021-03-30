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
#include "journal.hpp"

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

void JournalBase::operator()(Context ctx)
{
    for (const auto& n : index)
    {
        const auto& path = std::get<pathIndex>(n.first);
        const auto& pathMeta = std::get<pathMetaIndex>(n.second);
        const auto& propertyMeta = std::get<propertyMetaIndex>(n.second);
        const auto& storage = std::get<storageIndex>(n.second);
        const auto& value = std::get<valueIndex>(storage.get());

        if (!value.empty())
        {
            log(message, pathMeta, path, propertyMeta, value);
        }
    }
}
} // namespace monitoring
} // namespace dbus
} // namespace phosphor
