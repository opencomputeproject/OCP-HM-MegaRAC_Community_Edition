#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace phosphor
{
namespace smbus
{

class Smbus
{
  public:
    Smbus(){};

    int openI2cDev(int i2cbus, char* filename, size_t size, int quiet);

    int smbusInit(int smbus_num);

    void smbusClose(int smbus_num);

    int SendSmbusRWBlockCmdRAW(int smbus_num, int8_t device_addr,
                               uint8_t* tx_data, uint8_t tx_len,
                               uint8_t* rsp_data);
};

} // namespace smbus
} // namespace phosphor