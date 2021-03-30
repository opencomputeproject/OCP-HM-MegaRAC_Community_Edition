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

#pragma once
#include <experimental/filesystem>

namespace phosphor
{
namespace led
{
class SysfsLed
{
  public:
    SysfsLed(std::experimental::filesystem::path&& root) : root(std::move(root))
    {}
    SysfsLed() = delete;
    SysfsLed(const SysfsLed& other) = delete;

    virtual ~SysfsLed() = default;

    virtual unsigned long getBrightness();
    virtual void setBrightness(unsigned long value);
    virtual unsigned long getMaxBrightness();
    virtual std::string getTrigger();
    virtual void setTrigger(const std::string& trigger);
    virtual unsigned long getDelayOn();
    virtual void setDelayOn(unsigned long ms);
    virtual unsigned long getDelayOff();
    virtual void setDelayOff(unsigned long ms);

  protected:
    static constexpr char BRIGHTNESS[] = "brightness";
    static constexpr char MAX_BRIGHTNESS[] = "max_brightness";
    static constexpr char TRIGGER[] = "trigger";
    static constexpr char DELAY_ON[] = "delay_on";
    static constexpr char DELAY_OFF[] = "delay_off";

    std::experimental::filesystem::path root;
};
} // namespace led
} // namespace phosphor
