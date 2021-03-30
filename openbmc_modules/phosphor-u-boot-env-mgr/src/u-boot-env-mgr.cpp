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

#include "u-boot-env-mgr.hpp"
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <vector>
#include <unordered_map>
#include <xyz/openbmc_project/Common/error.hpp>

template <typename... ArgTypes>
static std::vector<std::string> executeCmd(const char* path,
                                           ArgTypes&&... tArgs)
{
    std::vector<std::string> stdOutput;
    boost::process::ipstream stdOutStream;
    boost::process::child execProg(path, const_cast<char*>(tArgs)...,
                                   boost::process::std_out > stdOutStream);
    std::string stdOutLine;

    while (stdOutStream && std::getline(stdOutStream, stdOutLine) &&
           !stdOutLine.empty())
    {
        stdOutput.emplace_back(stdOutLine);
    }

    execProg.wait();

    int retCode = execProg.exit_code();
    if (retCode)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Command execution failed",
            phosphor::logging::entry("PATH=%d", path),
            phosphor::logging::entry("RETURN_CODE:%d", retCode));
        phosphor::logging::elog<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
    }

    return stdOutput;
}

UBootEnvMgr::UBootEnvMgr(boost::asio::io_service& io_,
                         sdbusplus::asio::object_server& srv_,
                         std::shared_ptr<sdbusplus::asio::connection>& conn_) :
    io(io_),
    server(srv_), conn(conn_)
{
    iface = server.add_interface(uBootEnvMgrPath, uBootEnvMgrIface);
    iface->register_method("ReadAll", [this]() { return readAllVariable(); });
    iface->register_method("Read", [this](const std::string& key) {
        std::unordered_map<std::string, std::string> env = readAllVariable();
        auto it = env.find(key);
        if (it != env.end())
        {
            return it->second;
        }
        return std::string{};
    });

    iface->register_method(
        "Write", [this](const std::string& key, const std::string& value) {
            writeVariable(key, value);
        });
    iface->initialize(true);
}

std::unordered_map<std::string, std::string> UBootEnvMgr::readAllVariable()
{
    std::unordered_map<std::string, std::string> env;
    std::vector<std::string> output = executeCmd("/sbin/fw_printenv");
    for (const auto& entry : output)
    {
        size_t pos = entry.find("=");
        if (pos + 1 >= entry.size())
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Invalid U-Boot environment",
                phosphor::logging::entry("ENTRY=%s", entry.c_str()));
            continue;
        }
        // using string instead of string_view for null termination
        std::string key = entry.substr(0, pos);
        std::string value = entry.substr(pos + 1);
        env.emplace(key, value);
    }
    return env;
}

void UBootEnvMgr::writeVariable(const std::string& key,
                                const std::string& value)
{
    executeCmd("/sbin/fw_setenv", key.c_str(), value.c_str());
    return;
}
