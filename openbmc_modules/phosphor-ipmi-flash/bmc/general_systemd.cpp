/*
 * Copyright 2019 Google Inc.
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

#include "general_systemd.hpp"

#include "status.hpp"

#include <sdbusplus/bus.hpp>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace ipmi_flash
{

static constexpr auto systemdService = "org.freedesktop.systemd1";
static constexpr auto systemdRoot = "/org/freedesktop/systemd1";
static constexpr auto systemdInterface = "org.freedesktop.systemd1.Manager";
static constexpr auto jobInterface = "org.freedesktop.systemd1.Job";

bool SystemdNoFile::trigger()
{
    if (job)
    {
        std::fprintf(stderr, "Job alreading running %s: %s\n",
                     triggerService.c_str(), job->c_str());
        return false;
    }

    try
    {
        jobMonitor.emplace(
            bus,
            "type='signal',"
            "sender='org.freedesktop.systemd1',"
            "path='/org/freedesktop/systemd1',"
            "interface='org.freedesktop.systemd1.Manager',"
            "member='JobRemoved',",
            [&](sdbusplus::message::message& m) { this->match(m); });

        auto method = bus.new_method_call(systemdService, systemdRoot,
                                          systemdInterface, "StartUnit");
        method.append(triggerService);
        method.append(mode);

        sdbusplus::message::object_path obj_path;
        bus.call(method).read(obj_path);
        job = std::move(obj_path);
        std::fprintf(stderr, "Triggered %s mode %s: %s\n",
                     triggerService.c_str(), mode.c_str(), job->c_str());
        currentStatus = ActionStatus::running;
        return true;
    }
    catch (const std::exception& e)
    {
        job = std::nullopt;
        jobMonitor = std::nullopt;
        currentStatus = ActionStatus::failed;
        std::fprintf(stderr, "Failed to trigger %s mode %s: %s\n",
                     triggerService.c_str(), mode.c_str(), e.what());
        return false;
    }
}

void SystemdNoFile::abort()
{
    if (!job)
    {
        std::fprintf(stderr, "No running job %s\n", triggerService.c_str());
        return;
    }

    // Cancel the job
    auto cancel_req = bus.new_method_call(systemdService, job->c_str(),
                                          jobInterface, "Cancel");
    try
    {
        bus.call_noreply(cancel_req);
        std::fprintf(stderr, "Canceled %s: %s\n", triggerService.c_str(),
                     job->c_str());
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        std::fprintf(stderr, "Failed to cancel job %s %s: %s\n",
                     triggerService.c_str(), job->c_str(), ex.what());
    }
}

ActionStatus SystemdNoFile::status()
{
    return currentStatus;
}

const std::string& SystemdNoFile::getMode() const
{
    return mode;
}

void SystemdNoFile::match(sdbusplus::message::message& m)
{
    if (!job)
    {
        std::fprintf(stderr, "No running job %s\n", triggerService.c_str());
        return;
    }

    uint32_t job_id;
    sdbusplus::message::object_path job_path;
    std::string unit;
    std::string result;
    try
    {
        m.read(job_id, job_path, unit, result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::fprintf(stderr, "Bad JobRemoved signal %s: %s\n",
                     triggerService.c_str(), e.what());
        return;
    }

    if (*job != job_path.str)
    {
        return;
    }

    std::fprintf(stderr, "Job Finished %s %s: %s\n", triggerService.c_str(),
                 job->c_str(), result.c_str());
    jobMonitor = std::nullopt;
    job = std::nullopt;
    currentStatus =
        result == "done" ? ActionStatus::success : ActionStatus::failed;
}

std::unique_ptr<TriggerableActionInterface>
    SystemdNoFile::CreateSystemdNoFile(sdbusplus::bus::bus&& bus,
                                       const std::string& service,
                                       const std::string& mode)
{
    return std::make_unique<SystemdNoFile>(std::move(bus), service, mode);
}

std::unique_ptr<TriggerableActionInterface>
    SystemdWithStatusFile::CreateSystemdWithStatusFile(
        sdbusplus::bus::bus&& bus, const std::string& path,
        const std::string& service, const std::string& mode)
{
    return std::make_unique<SystemdWithStatusFile>(std::move(bus), path,
                                                   service, mode);
}

bool SystemdWithStatusFile::trigger()
{
    if (SystemdNoFile::status() != ActionStatus::running)
    {
        try
        {
            std::ofstream ofs;
            ofs.open(checkPath);
            ofs << "unknown";
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    return SystemdNoFile::trigger();
}

ActionStatus SystemdWithStatusFile::status()
{
    // Assume a status based on job execution if there is no file
    ActionStatus result = SystemdNoFile::status() == ActionStatus::running
                              ? ActionStatus::running
                              : ActionStatus::failed;

    std::ifstream ifs;
    ifs.open(checkPath);
    if (ifs.good())
    {
        /*
         * Check for the contents of the file, accepting:
         * running, success, or failed.
         */
        std::string status;
        ifs >> status;
        if (status == "running")
        {
            result = ActionStatus::running;
        }
        else if (status == "success")
        {
            result = ActionStatus::success;
        }
        else if (status == "failed")
        {
            result = ActionStatus::failed;
        }
        else
        {
            result = ActionStatus::unknown;
        }
    }

    return result;
}

} // namespace ipmi_flash
