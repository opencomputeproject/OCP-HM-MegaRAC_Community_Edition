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

#include "ChassisIntrusionSensor.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

extern "C"
{
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
}

static constexpr bool DEBUG = false;

static constexpr unsigned int intrusionSensorPollSec = 1;

// SMLink Status Register
const static constexpr size_t pchStatusRegIntrusion = 0x04;

// Status bit field masks
const static constexpr size_t pchRegMaskIntrusion = 0x01;

void ChassisIntrusionSensor::updateValue(const std::string newValue)
{
    // Take no action if value already equal
    // Same semantics as Sensor::updateValue(const double&)
    if (newValue == mValue)
    {
        return;
    }

    // indicate that it is internal set call
    mInternalSet = true;
    mIface->set_property("Status", newValue);
    mInternalSet = false;

    mValue = newValue;

    if (mOldValue == "Normal" && mValue != "Normal")
    {
        std::cerr << "save to SEL for intrusion assert event \n";
        // TODO: call add SEL log API, depends on patch #13956
        mOldValue = mValue;
    }
    else if (mOldValue != "Normal" && mValue == "Normal")
    {
        std::cerr << "save to SEL for intrusion de-assert event \n";
        // TODO: call add SEL log API, depends on patch #13956
        mOldValue = mValue;
    }
}

int ChassisIntrusionSensor::i2cReadFromPch(int busId, int slaveAddr)
{
    std::string i2cBus = "/dev/i2c-" + std::to_string(busId);

    int fd = open(i2cBus.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0)
    {
        std::cerr << "unable to open i2c device \n";
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE_FORCE, slaveAddr) < 0)
    {
        std::cerr << "unable to set device address\n";
        close(fd);
        return -1;
    }

    unsigned long funcs = 0;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        std::cerr << "not support I2C_FUNCS \n";
        close(fd);
        return -1;
    }

    if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA))
    {
        std::cerr << "not support I2C_FUNC_SMBUS_READ_BYTE_DATA \n";
        close(fd);
        return -1;
    }

    int statusValue;
    unsigned int statusMask = pchRegMaskIntrusion;
    unsigned int statusReg = pchStatusRegIntrusion;

    statusValue = i2c_smbus_read_byte_data(fd, statusReg);
    if (DEBUG)
    {
        std::cout << "\nRead bus " << busId << " addr " << slaveAddr
                  << ", value = " << statusValue << "\n";
    }

    close(fd);

    if (statusValue < 0)
    {
        std::cerr << "i2c_smbus_read_byte_data failed \n";
        return -1;
    }

    // Get status value with mask
    int newValue = statusValue & statusMask;

    if (DEBUG)
    {
        std::cout << "statusValue is " << statusValue << "\n";
        std::cout << "Intrusion sensor value is " << newValue << "\n";
    }

    return newValue;
}

void ChassisIntrusionSensor::pollSensorStatusByPch()
{
    // setting a new experation implicitly cancels any pending async wait
    mPollTimer.expires_from_now(
        boost::posix_time::seconds(intrusionSensorPollSec));

    mPollTimer.async_wait([&](const boost::system::error_code& ec) {
        // case of timer expired
        if (!ec)
        {
            int statusValue = i2cReadFromPch(mBusId, mSlaveAddr);
            std::string newValue = statusValue ? "HardwareIntrusion" : "Normal";

            if (newValue != "unknown" && mValue != newValue)
            {
                std::cout << "update value from " << mValue << " to "
                          << newValue << "\n";
                updateValue(newValue);
            }

            // trigger next polling
            pollSensorStatusByPch();
        }
        // case of being canceled
        else if (ec == boost::asio::error::operation_aborted)
        {
            std::cerr << "Timer of intrusion sensor is cancelled. Return \n";
            return;
        }
    });
}

void ChassisIntrusionSensor::readGpio()
{
    mGpioLine.event_read();
    auto value = mGpioLine.get_value();

    // set string defined in chassis redfish schema
    std::string newValue = value ? "HardwareIntrusion" : "Normal";

    if (DEBUG)
    {
        std::cout << "\nGPIO value is " << value << "\n";
        std::cout << "Intrusion sensor value is " << newValue << "\n";
    }

    if (newValue != "unknown" && mValue != newValue)
    {
        std::cout << "update value from " << mValue << " to " << newValue
                  << "\n";
        updateValue(newValue);
    }
}

void ChassisIntrusionSensor::pollSensorStatusByGpio(void)
{
    mGpioFd.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [this](const boost::system::error_code& ec) {
            if (ec == boost::system::errc::bad_file_descriptor)
            {
                return; // we're being destroyed
            }
            else if (ec)
            {
                std::cerr
                    << "Error on GPIO based intrusion sensor wait event\n";
            }
            else
            {
                readGpio();
            }
            pollSensorStatusByGpio();
        });
}

void ChassisIntrusionSensor::initGpioDeviceFile()
{
    mGpioLine = gpiod::find_line(mPinName);
    if (!mGpioLine)
    {
        std::cerr << "ChassisIntrusionSensor error finding gpio pin name: "
                  << mPinName << "\n";
        return;
    }

    try
    {

        mGpioLine.request(
            {"ChassisIntrusionSensor", gpiod::line_request::EVENT_BOTH_EDGES,
             mGpioInverted ? gpiod::line_request::FLAG_ACTIVE_LOW : 0});

        // set string defined in chassis redfish schema
        auto value = mGpioLine.get_value();
        std::string newValue = value ? "HardwareIntrusion" : "Normal";
        updateValue(newValue);

        auto gpioLineFd = mGpioLine.event_get_fd();
        if (gpioLineFd < 0)
        {
            std::cerr << "ChassisIntrusionSensor failed to get " << mPinName
                      << " fd\n";
            return;
        }

        mGpioFd.assign(gpioLineFd);
    }
    catch (std::system_error&)
    {
        std::cerr << "ChassisInrtusionSensor error requesting gpio pin name: "
                  << mPinName << "\n";
        return;
    }
}

int ChassisIntrusionSensor::setSensorValue(const std::string& req,
                                           std::string& propertyValue)
{
    if (!mInternalSet)
    {
        propertyValue = req;
        mOverridenState = true;
    }
    else if (!mOverridenState)
    {
        propertyValue = req;
    }
    return 1;
}

void ChassisIntrusionSensor::start(IntrusionSensorType type, int busId,
                                   int slaveAddr, bool gpioInverted)
{
    if (DEBUG)
    {
        std::cerr << "enter ChassisIntrusionSensor::start, type = " << type
                  << "\n";
        if (type == IntrusionSensorType::pch)
        {
            std::cerr << "busId = " << busId << ", slaveAddr = " << slaveAddr
                      << "\n";
        }
        else if (type == IntrusionSensorType::gpio)
        {
            std::cerr << "gpio pinName = " << mPinName
                      << ", gpioInverted = " << gpioInverted << "\n";
        }
    }

    if ((type == IntrusionSensorType::pch && busId == mBusId &&
         slaveAddr == mSlaveAddr) ||
        (type == IntrusionSensorType::gpio && gpioInverted == mGpioInverted &&
         mInitialized))
    {
        return;
    }

    mType = type;
    mBusId = busId;
    mSlaveAddr = slaveAddr;
    mGpioInverted = gpioInverted;

    if ((mType == IntrusionSensorType::pch && mBusId > 0 && mSlaveAddr > 0) ||
        (mType == IntrusionSensorType::gpio))
    {
        // initialize first if not initialized before
        if (!mInitialized)
        {
            mIface->register_property(
                "Status", mValue,
                [&](const std::string& req, std::string& propertyValue) {
                    return setSensorValue(req, propertyValue);
                });
            mIface->initialize();

            if (mType == IntrusionSensorType::gpio)
            {
                initGpioDeviceFile();
            }

            mInitialized = true;
        }

        // start polling value
        if (mType == IntrusionSensorType::pch)
        {
            pollSensorStatusByPch();
        }
        else if (mType == IntrusionSensorType::gpio && mGpioLine)
        {
            std::cerr << "Start polling intrusion sensors\n";
            pollSensorStatusByGpio();
        }
    }

    // invalid para, release resource
    else
    {
        if (mInitialized)
        {
            if (mType == IntrusionSensorType::pch)
            {
                mPollTimer.cancel();
            }
            else if (mType == IntrusionSensorType::gpio)
            {
                mGpioFd.close();
                if (mGpioLine)
                {
                    mGpioLine.release();
                }
            }
            mInitialized = false;
        }
    }
}

ChassisIntrusionSensor::ChassisIntrusionSensor(
    boost::asio::io_service& io,
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface) :
    mIface(iface),
    mType(IntrusionSensorType::gpio), mValue("unknown"), mOldValue("unknown"),
    mBusId(-1), mSlaveAddr(-1), mPollTimer(io), mGpioInverted(false),
    mGpioFd(io)
{}

ChassisIntrusionSensor::~ChassisIntrusionSensor()
{
    if (mType == IntrusionSensorType::pch)
    {
        mPollTimer.cancel();
    }
    else if (mType == IntrusionSensorType::gpio)
    {
        mGpioFd.close();
        if (mGpioLine)
        {
            mGpioLine.release();
        }
    }
}
