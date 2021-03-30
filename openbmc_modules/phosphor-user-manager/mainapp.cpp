/**
 * Copyright © 2016 IBM Corporation
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
#include <string>
#include "user_mgr.hpp"
#include "config.h"

// D-Bus root for user manager
constexpr auto USER_MANAGER_ROOT = "/xyz/openbmc_project/user";

int main(int argc, char** argv)
{
    auto bus = sdbusplus::bus::new_default();
    sdbusplus::server::manager::manager objManager(bus, USER_MANAGER_ROOT);

    phosphor::user::UserMgr userMgr(bus, USER_MANAGER_ROOT);

    // Claim the bus now
    bus.request_name(USER_MANAGER_BUSNAME);

    // Wait for client request
    while (true)
    {
        // process dbus calls / signals discarding unhandled
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
