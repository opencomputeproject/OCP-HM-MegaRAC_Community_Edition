/*
// Copyright (c) 2019 intel Corporation
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
#include <peci.h>

#include <cstdint>
#include <vector>

namespace peci_pcie
{
static constexpr char const* peciPCIeObject = "xyz.openbmc_project.PCIe";
static constexpr char const* peciPCIePath = "/xyz/openbmc_project/PCIe";
static constexpr char const* peciPCIeDeviceInterface =
    "xyz.openbmc_project.PCIe.Device";

static constexpr const int maxPCIBuses = 256;
static constexpr const int maxPCIDevices = 32;
static constexpr const int maxPCIFunctions = 8;

static constexpr const int peciCheckInterval = 10;
} // namespace peci_pcie
