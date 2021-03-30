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

#include <sdbusplus/asio/object_server.hpp>

static constexpr const char* uBootEnvMgrServiceName =
    "xyz.openbmc_project.U_Boot.Environment.Manager";
static constexpr const char* uBootEnvMgrIface =
    "xyz.openbmc_project.U_Boot.Environment.Manager";
static constexpr const char* uBootEnvMgrPath =
    "/xyz/openbmc_project/u_boot/environment/mgr";

class UBootEnvMgr
{
    boost::asio::io_service& io;
    sdbusplus::asio::object_server& server;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

    std::unordered_map<std::string, std::string> readAllVariable();
    void writeVariable(const std::string& key, const std::string& value);

  public:
    UBootEnvMgr(boost::asio::io_service& io,
                sdbusplus::asio::object_server& srv,
                std::shared_ptr<sdbusplus::asio::connection>& conn);
};
