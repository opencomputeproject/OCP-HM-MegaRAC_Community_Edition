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

#include "i2c.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

// TODO Add 16-bit I2C support in the furture
int i2cSet(uint8_t bus, uint8_t slaveAddr, uint8_t regAddr, uint8_t value)
{
    unsigned long funcs = 0;
    std::string devPath = "/dev/i2c-" + std::to_string(bus);

    int fd = ::open(devPath.c_str(), O_RDWR);
    if (fd < 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error in open!",
            phosphor::logging::entry("PATH=%s", devPath.c_str()),
            phosphor::logging::entry("SLAVEADDR=0x%x", slaveAddr));
        return -1;
    }

    if (::ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error in I2C_FUNCS!",
            phosphor::logging::entry("PATH=%s", devPath.c_str()),
            phosphor::logging::entry("SLAVEADDR=0x%x", slaveAddr));

        ::close(fd);
        return -1;
    }

    if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
    {

        phosphor::logging::log<phosphor::logging::level::ERR>(
            "i2c bus does not support write!",
            phosphor::logging::entry("PATH=%s", devPath.c_str()),
            phosphor::logging::entry("SLAVEADDR=0x%x", slaveAddr));
        ::close(fd);
        return -1;
    }

    if (::ioctl(fd, I2C_SLAVE_FORCE, slaveAddr) < 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error in I2C_SLAVE_FORCE!",
            phosphor::logging::entry("PATH=%s", devPath.c_str()),
            phosphor::logging::entry("SLAVEADDR=0x%x", slaveAddr));
        ::close(fd);
        return -1;
    }

    if (::i2c_smbus_write_byte_data(fd, regAddr, value) < 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error in i2c write!",
            phosphor::logging::entry("PATH=%s", devPath.c_str()),
            phosphor::logging::entry("SLAVEADDR=0x%x", slaveAddr));
        ::close(fd);
        return -1;
    }

    // TODO For testing, will remove the below debug loging in the future
    phosphor::logging::log<phosphor::logging::level::DEBUG>(
        "i2cset successfully",
        phosphor::logging::entry("PATH=%s", devPath.c_str()),
        phosphor::logging::entry("SLAVEADDR=0x%x", slaveAddr),
        phosphor::logging::entry("REGADDR=0x%x", regAddr),
        phosphor::logging::entry("VALUE=0x%x", value));
    ::close(fd);
    return 0;
}
