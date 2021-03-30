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
#include "elog.hpp"

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

void ElogBase::operator()(Context ctx)
{
    if (ctx == Context::START)
    {
        // No action should be taken as this call back is being called from
        // daemon Startup.
        return;
    }
    log();
}

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
