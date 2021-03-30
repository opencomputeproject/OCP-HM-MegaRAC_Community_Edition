/*
// Copyright (c) 2018 Intel Corporation
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
#include "utils.hpp"

static inline void checkAndThrowInternalFailure(boost::system::error_code& ec,
                                                const std::string& msg)
{
    if (ec)
    {
        std::string msgToLog = ec.message() + (msg.empty() ? "" : " - " + msg);
        phosphor::logging::log<phosphor::logging::level::ERR>(msgToLog.c_str());
        phosphor::logging::elog<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
    }
    return;
}

void systemdDaemonReload(
    const std::shared_ptr<sdbusplus::asio::connection>& conn,
    boost::asio::yield_context yield)
{
    boost::system::error_code ec;
    conn->yield_method_call<>(yield, ec, sysdService, sysdObjPath, sysdMgrIntf,
                              sysdReloadMethod);
    checkAndThrowInternalFailure(ec, "daemon-reload operation failed");
    return;
}

static inline uint32_t getJobId(const std::string& path)
{
    auto pos = path.rfind("/");
    if (pos == std::string::npos)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unable to get job id from job path");
        phosphor::logging::elog<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
    }
    return static_cast<uint32_t>(std::stoul(path.substr(pos + 1)));
}

void systemdUnitAction(const std::shared_ptr<sdbusplus::asio::connection>& conn,
                       boost::asio::yield_context yield,
                       const std::string& unitName,
                       const std::string& actionMethod)
{
    boost::system::error_code ec;
    auto jobPath = conn->yield_method_call<sdbusplus::message::object_path>(
        yield, ec, sysdService, sysdObjPath, sysdMgrIntf, actionMethod,
        unitName, sysdReplaceMode);
    checkAndThrowInternalFailure(ec,
                                 "Systemd operation failed, " + actionMethod);
    // Query the job till it doesn't exist anymore.
    // this way we guarantee that queued job id is done.
    // this is needed to make sure dependency list on units are
    // properly handled.
    while (1)
    {
        ec.clear();
        conn->yield_method_call<>(yield, ec, sysdService, sysdObjPath,
                                  sysdMgrIntf, sysdGetJobMethod,
                                  getJobId(jobPath.str));
        if (ec)
        {
            if (ec.value() == boost::system::errc::no_such_file_or_directory)
            {
                // Queued job is done, return now
                return;
            }
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Systemd operation failed for job query");
            phosphor::logging::elog<sdbusplus::xyz::openbmc_project::Common::
                                        Error::InternalFailure>();
        }
        boost::asio::steady_timer sleepTimer(conn->get_io_context());
        sleepTimer.expires_after(std::chrono::milliseconds(20));
        ec.clear();
        sleepTimer.async_wait(yield[ec]);
        checkAndThrowInternalFailure(ec, "Systemd operation timer error");
    }
    return;
}

void systemdUnitFilesStateChange(
    const std::shared_ptr<sdbusplus::asio::connection>& conn,
    boost::asio::yield_context yield, const std::vector<std::string>& unitFiles,
    const std::string& unitState, bool maskedState, bool enabledState)
{
    boost::system::error_code ec;

    if (unitState == stateMasked && !maskedState)
    {
        conn->yield_method_call<>(yield, ec, sysdService, sysdObjPath,
                                  sysdMgrIntf, "UnmaskUnitFiles", unitFiles,
                                  false);
        checkAndThrowInternalFailure(ec, "Systemd UnmaskUnitFiles() failed.");
    }
    else if (unitState != stateMasked && maskedState)
    {
        conn->yield_method_call<>(yield, ec, sysdService, sysdObjPath,
                                  sysdMgrIntf, "MaskUnitFiles", unitFiles,
                                  false, false);
        checkAndThrowInternalFailure(ec, "Systemd MaskUnitFiles() failed.");
    }
    ec.clear();
    if (unitState != stateEnabled && enabledState)
    {
        conn->yield_method_call<>(yield, ec, sysdService, sysdObjPath,
                                  sysdMgrIntf, "EnableUnitFiles", unitFiles,
                                  false, false);
        checkAndThrowInternalFailure(ec, "Systemd EnableUnitFiles() failed.");
    }
    else if (unitState != stateDisabled && !enabledState)
    {
        conn->yield_method_call<>(yield, ec, sysdService, sysdObjPath,
                                  sysdMgrIntf, "DisableUnitFiles", unitFiles,
                                  false);
        checkAndThrowInternalFailure(ec, "Systemd DisableUnitFiles() failed.");
    }
    return;
}
