#include "smbus.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>
#include <mutex>

#include "i2c.h"

#define MAX_I2C_BUS 30
static constexpr bool DEBUG = false;

static int fd[MAX_I2C_BUS] = {0};

namespace phosphor
{
namespace smbus
{

std::mutex gMutex;

int phosphor::smbus::Smbus::openI2cDev(int i2cbus, char* filename, size_t size,
                                       int quiet)
{
    int file;

    snprintf(filename, size, "/dev/i2c/%d", i2cbus);
    filename[size - 1] = '\0';
    file = open(filename, O_RDWR);

    if (file < 0 && (errno == ENOENT || errno == ENOTDIR))
    {
        sprintf(filename, "/dev/i2c-%d", i2cbus);
        file = open(filename, O_RDWR);
    }

    if (DEBUG)
    {
        if (file < 0 && !quiet)
        {
            if (errno == ENOENT)
            {
                fprintf(stderr,
                        "Error: Could not open file "
                        "`/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
                        i2cbus, i2cbus, strerror(ENOENT));
            }
            else
            {
                fprintf(stderr,
                        "Error: Could not open file "
                        "`%s': %s\n",
                        filename, strerror(errno));
                if (errno == EACCES)
                    fprintf(stderr, "Run as root?\n");
            }
        }
    }

    return file;
}

int phosphor::smbus::Smbus::smbusInit(int smbus_num)
{
    int res = 0;
    char filename[20];

    gMutex.lock();

    fd[smbus_num] = openI2cDev(smbus_num, filename, sizeof(filename), 0);
    if (fd[smbus_num] < 0)
    {
        gMutex.unlock();

        return -1;
    }

    res = fd[smbus_num];

    gMutex.unlock();

    return res;
}

void phosphor::smbus::Smbus::smbusClose(int smbus_num)
{
    close(fd[smbus_num]);
}

int phosphor::smbus::Smbus::SendSmbusRWBlockCmdRAW(int smbus_num,
                                                   int8_t device_addr,
                                                   uint8_t* tx_data,
                                                   uint8_t tx_len,
                                                   uint8_t* rsp_data)
{
    int res, res_len;
    unsigned char Rx_buf[I2C_DATA_MAX] = {0};

    Rx_buf[0] = 1;

    gMutex.lock();

    res = i2c_read_after_write(fd[smbus_num], device_addr, tx_len,
                               (unsigned char*)tx_data, I2C_DATA_MAX,
                               (unsigned char*)Rx_buf);

    if (res < 0)
    {
        fprintf(stderr, "Error: SendSmbusRWBlockCmdRAW failed\n");
    }

    res_len = Rx_buf[0] + 1;

    memcpy(rsp_data, Rx_buf, res_len);

    gMutex.unlock();

    return res;
}

} // namespace smbus
} // namespace phosphor
