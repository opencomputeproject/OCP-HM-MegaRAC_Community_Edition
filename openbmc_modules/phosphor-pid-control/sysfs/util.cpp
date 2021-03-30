/**
 * Copyright 2017 Google Inc.
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

#include "util.hpp"

#include <filesystem>
#include <iostream>
#include <string>

/*
 * There are two basic paths I want to support:
 * 1. /sys/class/hwmon/hwmon0/pwm1
 * 2. /sys/devices/platform/ahb/1e786000.pwm-tacho-controller/hwmon/<asterisk
 * asterisk>/pwm1
 *
 * In this latter case, I want to fill in that gap.  Assuming because it's this
 * path that it'll only have one directory there.
 */

static constexpr auto platform = "/sys/devices/platform/";
namespace fs = std::filesystem;

std::string FixupPath(std::string original)
{
    std::string::size_type n, x;

    /* TODO: Consider the merits of using regex for this. */
    n = original.find("**");
    x = original.find(platform);

    if ((n != std::string::npos) && (x != std::string::npos))
    {
        /* This path has some missing pieces and we support it. */
        std::string base = original.substr(0, n);
        std::string fldr;
        std::string f = original.substr(n + 2, original.size() - (n + 2));

        /* Equivalent to glob and grab 0th entry. */
        for (const auto& folder : fs::directory_iterator(base))
        {
            fldr = folder.path();
            break;
        }

        if (!fldr.length())
        {
            return original;
        }

        return fldr + f;
    }
    else
    {
        /* It'll throw an exception when we use it if it's still bad. */
        return original;
    }
}
