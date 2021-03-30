/**
 * Copyright © 2019 IBM Corporation
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

#include "sysfs.hpp"

#include <sys/param.h>

#include <cerrno>
#include <cstdlib>
#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::experimental::filesystem;

constexpr unsigned long MAX_BRIGHTNESS_VAL = 128;

class FakeSysfsLed : public phosphor::led::SysfsLed
{
  public:
    static FakeSysfsLed create()
    {
        static constexpr auto tmplt = "/tmp/FakeSysfsLed.XXXXXX";
        char buffer[MAXPATHLEN] = {0};

        strncpy(buffer, tmplt, sizeof(buffer) - 1);
        char* dir = mkdtemp(buffer);
        if (!dir)
            throw std::system_error(errno, std::system_category());

        return FakeSysfsLed(fs::path(dir));
    }

    ~FakeSysfsLed()
    {
        fs::remove_all(root);
    }

  private:
    FakeSysfsLed(fs::path&& path) : SysfsLed(std::move(path))
    {
        std::string attrs[4] = {BRIGHTNESS, TRIGGER, DELAY_ON, DELAY_OFF};
        for (const auto& attr : attrs)
        {
            fs::path p = root / attr;
            std::ofstream f(p, std::ios::out);
            f.exceptions(f.failbit);
        }

        fs::path p = root / MAX_BRIGHTNESS;
        std::ofstream f(p, std::ios::out);
        f.exceptions(f.failbit);
        f << MAX_BRIGHTNESS_VAL;
    }
};

TEST(Sysfs, getBrightness)
{
    constexpr unsigned long brightness = 127;
    FakeSysfsLed fsl = FakeSysfsLed::create();

    fsl.setBrightness(brightness);
    ASSERT_EQ(brightness, fsl.getBrightness());
}

TEST(Sysfs, getMaxBrightness)
{
    FakeSysfsLed fsl = FakeSysfsLed::create();

    ASSERT_EQ(MAX_BRIGHTNESS_VAL, fsl.getMaxBrightness());
}

TEST(Sysfs, getTrigger)
{
    constexpr auto trigger = "none";
    FakeSysfsLed fsl = FakeSysfsLed::create();

    fsl.setTrigger(trigger);
    ASSERT_EQ(trigger, fsl.getTrigger());
}

TEST(Sysfs, getDelayOn)
{
    constexpr unsigned long delayOn = 250;
    FakeSysfsLed fsl = FakeSysfsLed::create();

    fsl.setDelayOn(delayOn);
    ASSERT_EQ(delayOn, fsl.getDelayOn());
}

TEST(Sysfs, getDelayOff)
{
    constexpr unsigned long delayOff = 750;
    FakeSysfsLed fsl = FakeSysfsLed::create();

    fsl.setDelayOff(delayOff);
    ASSERT_EQ(delayOff, fsl.getDelayOff());
}
