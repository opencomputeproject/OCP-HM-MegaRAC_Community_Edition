/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <set>

/*
 * DbusPassiveRedundancy monitors the fan redundancy interface via dbus match
 * for changes. When the "Status" property changes to  Failed, all sensors in
 * the interface's "Collection" property are added to the failed member. This
 * set can then be queried by a sensor to see if they are part of a failed
 * redundancy collection. When this happens, they get marked as failed.
 */

class DbusPassiveRedundancy
{
  public:
    DbusPassiveRedundancy(sdbusplus::bus::bus& bus);
    const std::set<std::string>& getFailed(void);

  private:
    void populateFailures(void);

    sdbusplus::bus::match::match match;
    std::set<std::string> failed;
    sdbusplus::bus::bus& passiveBus;
};
