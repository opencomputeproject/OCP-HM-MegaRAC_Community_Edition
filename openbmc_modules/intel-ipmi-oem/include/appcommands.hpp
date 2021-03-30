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
#include <ipmid/api.hpp>
namespace ipmi
{

#pragma pack(push, 1)
typedef struct
{
    std::string platform;
    uint8_t major;
    uint8_t minor;
    uint32_t buildNo;
    std::string openbmcHash;
    std::string metaHash;
} MetaRevision;
#pragma pack(pop)

static constexpr const char* versionPurposeBMC =
    "xyz.openbmc_project.Software.Version.VersionPurpose.BMC";
static constexpr const char* versionPurposeME =
    "xyz.openbmc_project.Software.Version.VersionPurpose.ME";

extern int getActiveSoftwareVersionInfo(ipmi::Context::ptr ctx,
                                        const std::string& reqVersionPurpose,
                                        std::string& version);
extern std::optional<MetaRevision> convertIntelVersion(std::string& s);
} // namespace ipmi
