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

#pragma once
#include "common.hpp"
#include "gpio.hpp"
#include "xyz/openbmc_project/Chassis/Buttons/Power/server.hpp"
#include "xyz/openbmc_project/Chassis/Common/error.hpp"

#include <unistd.h>

#include <chrono>
#include <phosphor-logging/elog-errors.hpp>

const static constexpr char* POWER_BUTTON = "POWER_BUTTON";

struct PowerButton
    : sdbusplus::server::object::object<
          sdbusplus::xyz::openbmc_project::Chassis::Buttons::server::Power>
{

    PowerButton(sdbusplus::bus::bus& bus, const char* path, EventPtr& event,
                sd_event_io_handler_t handler = PowerButton::EventHandler) :
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Chassis::Buttons::server::Power>(
            bus, path),
        fd(-1), bus(bus), event(event), callbackHandler(handler)
    {

        int ret = -1;

        // config gpio
        ret = ::configGpio(POWER_BUTTON, &fd, bus);
        if (ret < 0)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "POWER_BUTTON: failed to config GPIO");
            throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                IOError();
        }

        char buf;
        ::read(fd, &buf, sizeof(buf));

        ret = sd_event_add_io(event.get(), nullptr, fd, EPOLLPRI,
                              callbackHandler, this);
        if (ret < 0)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "POWER_BUTTON: failed to add to event loop");
            ::closeGpio(fd);
            throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                IOError();
        }
    }

    ~PowerButton()
    {
        ::closeGpio(fd);
    }

    void simPress() override;
    void simLongPress() override;

    static const char* getGpioName()
    {
        return POWER_BUTTON;
    }

    void updatePressedTime()
    {
        pressedTime = std::chrono::steady_clock::now();
    }

    auto getPressTime() const
    {
        return pressedTime;
    }

    static int EventHandler(sd_event_source* es, int fd, uint32_t revents,
                            void* userdata)
    {

        int n = -1;
        char buf = '0';

        if (!userdata)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "POWER_BUTTON: userdata null!");
            throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                IOError();
        }

        PowerButton* powerButton = static_cast<PowerButton*>(userdata);

        if (!powerButton)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "POWER_BUTTON: null pointer!");
            throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                IOError();
        }

        n = ::lseek(fd, 0, SEEK_SET);

        if (n < 0)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "POWER_BUTTON: lseek error!");
            throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                IOError();
        }

        n = ::read(fd, &buf, sizeof(buf));
        if (n < 0)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "POWER_BUTTON: read error!");
            throw sdbusplus::xyz::openbmc_project::Chassis::Common::Error::
                IOError();
        }

        if (buf == '0')
        {
            phosphor::logging::log<phosphor::logging::level::DEBUG>(
                "POWER_BUTTON: pressed");

            powerButton->updatePressedTime();
            // emit pressed signal
            powerButton->pressed();
        }
        else
        {
            phosphor::logging::log<phosphor::logging::level::DEBUG>(
                "POWER_BUTTON: released");

            auto now = std::chrono::steady_clock::now();
            auto d = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - powerButton->getPressTime());

            if (d > std::chrono::milliseconds(LONG_PRESS_TIME_MS))
            {
                powerButton->pressedLong();
            }
            else
            {
                // released
                powerButton->released();
            }
        }

        return 0;
    }

  private:
    int fd;
    sdbusplus::bus::bus& bus;
    EventPtr& event;
    sd_event_io_handler_t callbackHandler;
    decltype(std::chrono::steady_clock::now()) pressedTime;
};
