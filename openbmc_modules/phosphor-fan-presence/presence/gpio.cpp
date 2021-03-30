/**
 * Copyright © 2017 IBM Corporation
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
#include "gpio.hpp"

#include "rpolicy.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <sdeventplus/event.hpp>
#include <xyz/openbmc_project/Common/Callout/error.hpp>

#include <functional>
#include <tuple>

namespace phosphor
{
namespace fan
{
namespace presence
{

Gpio::Gpio(const std::string& physDevice, const std::string& device,
           unsigned int physPin) :
    currentState(false),
    evdevfd(open(device.c_str(), O_RDONLY | O_NONBLOCK)),
    evdev(evdevpp::evdev::newFromFD(evdevfd())), phys(physDevice), pin(physPin)
{}

bool Gpio::start()
{
    source.emplace(sdeventplus::Event::get_default(), evdevfd(), EPOLLIN,
                   std::bind(&Gpio::ioCallback, this));
    currentState = present();
    return currentState;
}

void Gpio::stop()
{
    source.reset();
}

bool Gpio::present()
{
    return evdev.fetch(EV_KEY, pin) != 0;
}

void Gpio::fail()
{
    using namespace sdbusplus::xyz::openbmc_project::Common::Callout::Error;
    using namespace phosphor::logging;
    using namespace xyz::openbmc_project::Common::Callout;

    report<sdbusplus::xyz::openbmc_project::Common::Callout::Error::GPIO>(
        GPIO::CALLOUT_GPIO_NUM(pin), GPIO::CALLOUT_ERRNO(0),
        GPIO::CALLOUT_DEVICE_PATH(phys.c_str()));
}

void Gpio::ioCallback()
{
    unsigned int type, code, value;

    std::tie(type, code, value) = evdev.next();
    if (type != EV_KEY || code != pin)
    {
        return;
    }

    bool newState = value != 0;

    if (currentState != newState)
    {
        getPolicy().stateChanged(newState, *this);
        currentState = newState;
    }
}
} // namespace presence
} // namespace fan
} // namespace phosphor
