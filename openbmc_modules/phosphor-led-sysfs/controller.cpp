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

#include "argument.hpp"
#include "physical.hpp"
#include "sysfs.hpp"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <iostream>
#include <string>

static void ExitWithError(const char* err, char** argv)
{
    phosphor::led::ArgumentParser::usage(argv);
    std::cerr << std::endl;
    std::cerr << "ERROR: " << err << std::endl;
    exit(-1);
}

struct LedDescr
{
    std::string devicename;
    std::string color;
    std::string function;
};

/** @brief parse LED name in sysfs
 *  Parse sysfs LED name in format "devicename:colour:function"
 *  or "devicename:colour" or "devicename" and sets corresponding
 *  fields in LedDescr struct.
 *
 *  @param[in] name      - LED name in sysfs
 *  @param[out] ledDescr - LED description
 */
void getLedDescr(const std::string& name, LedDescr& ledDescr)
{
    std::vector<std::string> words;
    boost::split(words, name, boost::is_any_of(":"));
    try
    {
        ledDescr.devicename = words.at(0);
        ledDescr.color = words.at(1);
        ledDescr.function = words.at(2);
    }
    catch (const std::out_of_range&)
    {
        return;
    }
}

/** @brief generates LED DBus name from LED description
 *
 *  @param[in] name      - LED description
 *  @return              - DBus LED name
 */
std::string getDbusName(const LedDescr& ledDescr)
{
    std::vector<std::string> words;
    words.emplace_back(ledDescr.devicename);
    if (!ledDescr.function.empty())
        words.emplace_back(ledDescr.function);
    if (!ledDescr.color.empty())
        words.emplace_back(ledDescr.color);
    return boost::join(words, "_");
}

int main(int argc, char** argv)
{
    namespace fs = std::experimental::filesystem;

    // Read arguments.
    auto options = phosphor::led::ArgumentParser(argc, argv);

    // Parse out Path argument.
    auto path = std::move((options)["path"]);
    if (path == phosphor::led::ArgumentParser::empty_string)
    {
        ExitWithError("Path not specified.", argv);
    }

    // If the LED has a hyphen in the name like: "one-two", then it gets passed
    // as /one/two/ as opposed to /one-two to the service file. There is a
    // change needed in systemd to solve this issue and hence putting in this
    // work-around.

    // Since this application always gets invoked as part of a udev rule,
    // it is always guaranteed to get /sys/class/leds/one/two
    // and we can go beyond leds/ to get the actual LED name.
    // Refer: systemd/systemd#5072

    // On an error, this throws an exception and terminates.
    auto name = path.substr(strlen(DEVPATH));

    // LED names may have a hyphen and that would be an issue for
    // dbus paths and hence need to convert them to underscores.
    std::replace(name.begin(), name.end(), '/', '-');
    path = DEVPATH + name;

    // Convert to lowercase just in case some are not and that
    // we follow lowercase all over
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    // LED names may have a hyphen and that would be an issue for
    // dbus paths and hence need to convert them to underscores.
    std::replace(name.begin(), name.end(), '-', '_');

    // Convert LED name in sysfs into DBus name
    LedDescr ledDescr;
    getLedDescr(name, ledDescr);
    name = getDbusName(ledDescr);

    // Unique bus name representing a single LED.
    auto busName = std::string(BUSNAME) + '.' + name;
    auto objPath = std::string(OBJPATH) + '/' + name;

    // Get a handle to system dbus.
    auto bus = sdbusplus::bus::new_default();

    // Add systemd object manager.
    sdbusplus::server::manager::manager(bus, objPath.c_str());

    // Create the Physical LED objects for directing actions.
    // Need to save this else sdbusplus destructor will wipe this off.
    phosphor::led::SysfsLed sled{fs::path(path)};
    phosphor::led::Physical led(bus, objPath, sled, ledDescr.color);

    /** @brief Claim the bus */
    bus.request_name(busName.c_str());

    /** @brief Wait for client requests */
    while (true)
    {
        // Handle dbus message / signals discarding unhandled
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
