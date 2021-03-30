#pragma once

#include <chrono>
#include <exception>
#include <fstream>
#include <string>

namespace sysfs
{

inline std::string make_sysfs_path(const std::string& path,
                                   const std::string& type,
                                   const std::string& id,
                                   const std::string& entry)
{
    using namespace std::literals;

    if (entry.empty())
    {
        return path + "/"s + type + id;
    }

    return path + "/"s + type + id + "_"s + entry;
}

/** @brief Return the path to the phandle file matching value in io-channels.
 *
 *  This function will take two passed in paths.
 *  One path is used to find the io-channels file.
 *  The other path is used to find the phandle file.
 *  The 4 byte phandle value is read from the phandle file(s).
 *  The 4 byte phandle value and 4 byte index value is read from io-channels.
 *  When a match is found, the path to the matching phandle file is returned.
 *
 *  @param[in] iochanneldir - Path to file for getting phandle from io-channels
 *  @param[in] phandledir - Path to use for reading from phandle file
 *
 *  @return Path to phandle file with value matching that in io-channels
 */
std::string findPhandleMatch(const std::string& iochanneldir,
                             const std::string& phandledir);

/** @brief Find hwmon instances from an open-firmware device tree path
 *
 *  Look for a matching hwmon instance given an
 *  open firmware device path.
 *
 *  @param[in] ofNode- The open firmware device path.
 *
 *  @returns[in] - The hwmon instance path or an empty
 *                 string if no match is found.
 */
std::string findHwmonFromOFPath(const std::string& ofNode);

/** @brief Find hwmon instances from a device path
 *
 *  Look for a matching hwmon instance given a device path that
 *  starts with /devices.  This path is the DEVPATH udev attribute
 *  for the device except it has the '/hwmon/hwmonN' stripped off.
 *
 *  @param[in] devPath - The device path.
 *
 *  @return - The hwmon instance path or an empty
 *            string if no match is found.
 */
std::string findHwmonFromDevPath(const std::string& devPath);

/** @brief Return the path to use for a call out.
 *
 *  Return an empty string if a callout path cannot be
 *  found.
 *
 *  @param[in] instancePath - /sys/class/hwmon/hwmon<N> path.
 *
 *  @return Path to use for call out
 */
std::string findCalloutPath(const std::string& instancePath);

} // namespace sysfs

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
