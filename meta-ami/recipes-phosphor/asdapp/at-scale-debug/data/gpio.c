/*
Copyright (c) 2019, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "gpio.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
// clang-format off
#include <safe_str_lib.h>
// clang-format on

#define GPIO_EDGE_NONE_STRING "none"
#define GPIO_EDGE_RISING_STRING "rising"
#define GPIO_EDGE_FALLING_STRING "falling"
#define GPIO_EDGE_BOTH_STRING "both"

#define GPIO_DIRECTION_IN_STRING "in"
#define GPIO_DIRECTION_OUT_STRING "out"
#define GPIO_DIRECTION_HIGH_STRING "high"
#define GPIO_DIRECTION_LOW_STRING "low"
#define BUFF_SIZE 48

STATUS gpio_export(int gpio, int* gpio_fd)
{
    int fd;
    char buf[BUFF_SIZE];
    int ia[1];
    STATUS result = ST_OK;
    if (!gpio_fd)
        return ST_ERR;
    ia[0] = gpio;
    sprintf_s(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", ia, 1);
    *gpio_fd = open(buf, O_WRONLY);
    if (*gpio_fd == -1)
    {
        fd = open("/sys/class/gpio/export", O_WRONLY);
        if (fd >= 0)
        {
            sprintf_s(buf, sizeof(buf), "%d", ia, 1);
            if (write(fd, buf, strlen(buf)) < 0)
            {
                result = ST_ERR;
            }
            else
            {
                sprintf_s(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", ia,
                          1);
                *gpio_fd = open(buf, O_RDWR);
                if (*gpio_fd == -1)
                    result = ST_ERR;
            }
            close(fd);
        }
        else
        {
            result = ST_ERR;
        }
    }
    return result;
}

STATUS gpio_unexport(int gpio)
{
    int fd;
    char buf[BUFF_SIZE];
    STATUS result = ST_OK;
    int ia[1];
    ia[0] = gpio;
    sprintf_s(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", ia, 1);
    fd = open(buf, O_WRONLY);
    if (fd >= 0)
    {
        // the gpio exists
        close(fd);
        fd = open("/sys/class/gpio/unexport", O_WRONLY);
        int ia[1];
        ia[0] = gpio;

        if (fd >= 0)
        {
            sprintf_s(buf, sizeof(buf), "%d", ia, 1);
            if (write(fd, buf, strlen(buf)) < 0)
            {
                result = ST_ERR;
            }
            close(fd);
        }
        else
        {
            result = ST_ERR;
        }
    }
    return result;
}

STATUS gpio_get_value(int fd, int* value)
{
    STATUS result = ST_ERR;
    char ch;

    if (value && fd >= 0)
    {
        lseek(fd, 0, SEEK_SET);
        read(fd, &ch, 1);
        *value = ch != '0' ? 1 : 0;
        result = ST_OK;
    }
    return result;
}

STATUS gpio_set_value(int fd, int value)
{
    STATUS result = ST_ERR;

    if (fd >= 0)
    {
        lseek(fd, 0, SEEK_SET);
        ssize_t written = write(fd, value == 1 ? "1" : "0", sizeof(char));
        if (written == sizeof(char))
        {
            result = ST_OK;
        }
    }
    return result;
}

STATUS gpio_set_edge(int gpio, GPIO_EDGE edge)
{
    int fd;
    char buf[BUFF_SIZE];
    int ia[1];
    ia[0] = gpio;
    STATUS result = ST_ERR;

    sprintf_s((buf), sizeof(buf), "/sys/class/gpio/gpio%d/edge", ia, 1);
    fd = open(buf, O_WRONLY);
    if (fd >= 0)
    {
        if (edge == GPIO_EDGE_NONE)
            write(fd, GPIO_EDGE_NONE_STRING, strlen(GPIO_EDGE_NONE_STRING));
        else if (edge == GPIO_EDGE_RISING)
            write(fd, GPIO_EDGE_RISING_STRING, strlen(GPIO_EDGE_RISING_STRING));
        else if (edge == GPIO_EDGE_FALLING)
            write(fd, GPIO_EDGE_FALLING_STRING,
                  strlen(GPIO_EDGE_FALLING_STRING));
        else if (edge == GPIO_EDGE_BOTH)
            write(fd, GPIO_EDGE_BOTH_STRING, strlen(GPIO_EDGE_BOTH_STRING));
        close(fd);
        result = ST_OK;
    }
    return result;
}

STATUS gpio_set_direction(int gpio, GPIO_DIRECTION direction)
{
    int fd;
    char buf[BUFF_SIZE];
    int ia[1];
    ia[0] = gpio;
    STATUS result = ST_ERR;
    sprintf_s(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", ia, 1);
    fd = open(buf, O_WRONLY);
    if (fd >= 0)
    {
        if (direction == GPIO_DIRECTION_IN)
            write(fd, GPIO_DIRECTION_IN_STRING,
                  strlen(GPIO_DIRECTION_IN_STRING));
        else if (direction == GPIO_DIRECTION_OUT)
            write(fd, GPIO_DIRECTION_OUT_STRING,
                  strlen(GPIO_DIRECTION_OUT_STRING));
        else if (direction == GPIO_DIRECTION_HIGH)
            write(fd, GPIO_DIRECTION_HIGH_STRING,
                  strlen(GPIO_DIRECTION_HIGH_STRING));
        else if (direction == GPIO_DIRECTION_LOW)
            write(fd, GPIO_DIRECTION_LOW_STRING,
                  strlen(GPIO_DIRECTION_LOW_STRING));
        close(fd);
        result = ST_OK;
    }
    return result;
}

STATUS gpio_set_active_low(int gpio, bool active_low)
{
    int fd;
    char buf[BUFF_SIZE];
    STATUS result = ST_ERR;
    int ia[1];
    ia[0] = gpio;
    sprintf_s(buf, sizeof(buf), "/sys/class/gpio/gpio%d/active_low", ia, 1);
    fd = open(buf, O_WRONLY);
    if (fd >= 0)
    {
        if (write(fd, active_low ? "1" : "0", 1) == 1)
        {
            result = ST_OK;
        }
        close(fd);
    }
    return result;
}
