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

#include "sysfs.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <thread>

using namespace std::string_literals;
namespace fs = std::filesystem;

namespace sysfs
{

static const auto emptyString = ""s;
static constexpr auto ofRoot = "/sys/firmware/devicetree/base";

std::string findPhandleMatch(const std::string& iochanneldir,
                             const std::string& phandledir)
{
    // TODO: At the moment this method only supports device trees
    // with iio-hwmon nodes with a single sensor.  Typically
    // device trees are defined with all the iio sensors in a
    // single iio-hwmon node so it would be nice to add support
    // for lists of phandles (with variable sized entries) via
    // libfdt or something like that, so that users are not
    // forced into implementing unusual looking device trees
    // with multiple iio-hwmon nodes - one for each sensor.

    fs::path ioChannelsPath{iochanneldir};
    ioChannelsPath /= "io-channels";

    if (!fs::exists(ioChannelsPath))
    {
        return emptyString;
    }

    uint32_t ioChannelsValue;
    std::ifstream ioChannelsFile(ioChannelsPath);

    ioChannelsFile.read(reinterpret_cast<char*>(&ioChannelsValue),
                        sizeof(ioChannelsValue));

    for (const auto& ofInst : fs::recursive_directory_iterator(phandledir))
    {
        auto path = ofInst.path();
        if ("phandle" != path.filename())
        {
            continue;
        }
        std::ifstream pHandleFile(path);
        uint32_t pHandleValue;

        pHandleFile.read(reinterpret_cast<char*>(&pHandleValue),
                         sizeof(pHandleValue));

        if (ioChannelsValue == pHandleValue)
        {
            return path;
        }
    }

    return emptyString;
}

std::string findCalloutPath(const std::string& instancePath)
{
    // Follow the hwmon instance (/sys/class/hwmon/hwmon<N>)
    // /sys/devices symlink.
    fs::path devPath{instancePath};
    devPath /= "device";

    try
    {
        devPath = fs::canonical(devPath);
    }
    catch (const std::system_error& e)
    {
        return emptyString;
    }

    // See if the device is backed by the iio-hwmon driver.
    fs::path p{devPath};
    p /= "driver";
    p = fs::canonical(p);

    if (p.filename() != "iio_hwmon")
    {
        // Not backed by iio-hwmon.  The device pointed to
        // is the callout device.
        return devPath;
    }

    // Find the DT path to the iio-hwmon platform device.
    fs::path ofDevPath{devPath};
    ofDevPath /= "of_node";

    try
    {
        ofDevPath = fs::canonical(ofDevPath);
    }
    catch (const std::system_error& e)
    {
        return emptyString;
    }

    // Search /sys/bus/iio/devices for the phandle in io-channels.
    // If a match is found, use the corresponding /sys/devices
    // iio device as the callout device.
    static constexpr auto iioDevices = "/sys/bus/iio/devices";
    for (const auto& iioDev : fs::recursive_directory_iterator(iioDevices))
    {
        p = iioDev.path();
        p /= "of_node";

        try
        {
            p = fs::canonical(p);
        }
        catch (const std::system_error& e)
        {
            continue;
        }

        auto match = findPhandleMatch(ofDevPath, p);
        auto n = match.rfind('/');
        if (n != std::string::npos)
        {
            // This is the iio device referred to by io-channels.
            // Remove iio:device<N>.
            try
            {
                return fs::canonical(iioDev).parent_path();
            }
            catch (const std::system_error& e)
            {
                return emptyString;
            }
        }
    }

    return emptyString;
}

std::string findHwmonFromOFPath(const std::string& ofNode)
{
    static constexpr auto hwmonRoot = "/sys/class/hwmon";

    auto fullOfPath = fs::path(ofRoot) / fs::path(ofNode).relative_path();

    for (const auto& hwmonInst : fs::directory_iterator(hwmonRoot))
    {
        auto path = hwmonInst.path();
        path /= "of_node";

        try
        {
            path = fs::canonical(path);
        }
        catch (const std::system_error& e)
        {
            // realpath may encounter ENOENT (Hwmon
            // instances have a nasty habit of
            // going away without warning).
            continue;
        }

        if (path == fullOfPath)
        {
            return hwmonInst.path();
        }

        // Try to find HWMON instance via phandle values.
        // Used for IIO device drivers.
        auto matchpath = findPhandleMatch(path, fullOfPath);
        if (!matchpath.empty())
        {
            return hwmonInst.path();
        }
    }

    return emptyString;
}

std::string findHwmonFromDevPath(const std::string& devPath)
{
    fs::path p{"/sys"};
    p /= fs::path(devPath).relative_path();
    p /= "hwmon";

    try
    {
        // This path is also used as a filesystem path to an environment
        // file, and that has issues with ':'s in the path so they've
        // been converted to '--'s.  Convert them back now.
        size_t pos = 0;
        std::string path = p;
        while ((pos = path.find("--")) != std::string::npos)
        {
            path.replace(pos, 2, ":");
        }

        auto dir_iter = fs::directory_iterator(path);
        auto hwmonInst = std::find_if(
            dir_iter, end(dir_iter), [](const fs::directory_entry& d) {
                return (d.path().filename().string().find("hwmon") !=
                        std::string::npos);
            });
        if (hwmonInst != end(dir_iter))
        {
            return hwmonInst->path();
        }
    }
    catch (const std::exception& e)
    {
        using namespace phosphor::logging;
        log<level::ERR>("Unable to find hwmon directory from the dev path",
                        entry("PATH=%s", devPath.c_str()));
    }
    return emptyString;
}

} // namespace sysfs

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
