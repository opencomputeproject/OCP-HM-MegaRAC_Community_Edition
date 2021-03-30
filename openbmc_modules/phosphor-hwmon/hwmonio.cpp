/**
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

#include "hwmonio.hpp"

#include "sysfs.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <thread>

namespace hwmonio
{

int64_t FileSystem::read(const std::string& path) const
{
    int64_t val;
    std::ifstream ifs;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                   std::ifstream::eofbit);

    errno = 0;
    if (!ifs.is_open())
        ifs.open(path);
    ifs.clear();
    ifs.seekg(0);
    ifs >> val;

    return val;
}

void FileSystem::write(const std::string& path, uint32_t value) const
{
    std::ofstream ofs;
    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit |
                   std::ofstream::eofbit);

    errno = 0;
    if (!ofs.is_open())
        ofs.open(path);
    ofs.clear();
    ofs.seekp(0);
    ofs << value;
    ofs.flush();
}

FileSystem fileSystemImpl;

static constexpr auto retryableErrors = {
    /*
     * Retry on bus or device errors or timeouts in case
     * they are transient.
     */
    EIO,
    ETIMEDOUT,

    /*
     * Retry CRC errors.
     */
    EBADMSG,

    /*
     * Some hwmon drivers do this when they aren't ready
     * instead of blocking.  Retry.
     */
    EAGAIN,
    /*
     * We'll see this when for example i2c devices are
     * unplugged but the driver is still bound.  Retry
     * rather than exit on the off chance the device is
     * plugged back in and the driver doesn't do a
     * remove/probe.  If a remove does occur, we'll
     * eventually get ENOENT.
     */
    ENXIO,

    /*
     * Some devices return this when they are busy doing
     * something else.  Even if being busy isn't the cause,
     * a retry still gives this app a shot at getting data
     * as opposed to failing out on the first try.
     */
    ENODATA,

    /*
     * Some devices return this if the hardware is being
     * powered off in a normal manner, as incomplete data
     * is received. Retrying allows time for the system to
     * clean up driver devices, or in the event of a real
     * failure, attempt to get the rest of the data.
     */
    EMSGSIZE,
};

HwmonIO::HwmonIO(const std::string& path, const FileSystemInterface* intf) :
    _p(path), _intf(intf)
{
}

int64_t HwmonIO::read(const std::string& type, const std::string& id,
                      const std::string& sensor, size_t retries,
                      std::chrono::milliseconds delay) const
{
    int64_t val;
    auto fullPath = sysfs::make_sysfs_path(_p, type, id, sensor);

    while (true)
    {
        try
        {
            val = _intf->read(fullPath);
        }
        catch (const std::exception& e)
        {
            auto rc = errno;

            if (!rc)
            {
                throw;
            }

            if (rc == ENOENT || rc == ENODEV)
            {
                // If the directory or device disappeared then this application
                // should gracefully exit.  There are race conditions between
                // the unloading of a hwmon driver and the stopping of this
                // service by systemd.  To prevent this application from falsely
                // failing in these scenarios, it will simply exit if the
                // directory or file can not be found.  It is up to the user(s)
                // of this provided hwmon object to log the appropriate errors
                // if the object disappears when it should not.
                exit(0);
            }

            if (0 == std::count(retryableErrors.begin(), retryableErrors.end(),
                                rc) ||
                !retries)
            {
                // Not a retryable error or out of retries.
#ifdef NEGATIVE_ERRNO_ON_FAIL
                return -rc;
#endif

                // Work around GCC bugs 53984 and 66145 for callers by
                // explicitly raising system_error here.
                throw std::system_error(rc, std::generic_category());
            }

            --retries;
            std::this_thread::sleep_for(delay);
            continue;
        }
        break;
    }

    return val;
}

void HwmonIO::write(uint32_t val, const std::string& type,
                    const std::string& id, const std::string& sensor,
                    size_t retries, std::chrono::milliseconds delay) const

{
    auto fullPath = sysfs::make_sysfs_path(_p, type, id, sensor);

    // See comments in the read method for an explanation of the odd exception
    // handling behavior here.

    while (true)
    {
        try
        {
            _intf->write(fullPath, val);
        }
        catch (const std::exception& e)
        {
            auto rc = errno;

            if (!rc)
            {
                throw;
            }

            if (rc == ENOENT)
            {
                exit(0);
            }

            if (0 == std::count(retryableErrors.begin(), retryableErrors.end(),
                                rc) ||
                !retries)
            {
                // Not a retryable error or out of retries.

                // Work around GCC bugs 53984 and 66145 for callers by
                // explicitly raising system_error here.
                throw std::system_error(rc, std::generic_category());
            }

            --retries;
            std::this_thread::sleep_for(delay);
            continue;
        }
        break;
    }
}

std::string HwmonIO::path() const
{
    return _p;
}

} // namespace hwmonio
