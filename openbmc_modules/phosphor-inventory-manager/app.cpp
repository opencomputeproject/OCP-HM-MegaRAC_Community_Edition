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
#include "config.h"

#include "manager.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <sdbusplus/bus.hpp>

int main(int argc, char* argv[])
{
    try
    {
        phosphor::inventory::manager::Manager manager(
            sdbusplus::bus::new_system(), BUSNAME, INVENTORY_ROOT, IFACE);
        manager.run();
        exit(EXIT_SUCCESS);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    exit(EXIT_FAILURE);
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
