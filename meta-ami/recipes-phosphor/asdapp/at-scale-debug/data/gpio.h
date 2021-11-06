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

#ifndef __GPIO_H_
#define __GPIO_H_

#include <stdbool.h>

#include "asd_common.h"

#define ALL_GPIO_EDGES(FUNC)                                                   \
    FUNC(GPIO_EDGE_NONE)                                                       \
    FUNC(GPIO_EDGE_RISING)                                                     \
    FUNC(GPIO_EDGE_FALLING)                                                    \
    FUNC(GPIO_EDGE_BOTH)

#define ALL_GPIO_DIRECTIONS(FUNC)                                              \
    FUNC(GPIO_DIRECTION_IN)                                                    \
    FUNC(GPIO_DIRECTION_OUT)                                                   \
    FUNC(GPIO_DIRECTION_HIGH)                                                  \
    FUNC(GPIO_DIRECTION_LOW)

typedef enum
{
    ALL_GPIO_EDGES(TO_ENUM)
} GPIO_EDGE;

static const char* GPIO_EDGE_STRINGS[] = {ALL_GPIO_EDGES(TO_STRING)};

typedef enum
{
    ALL_GPIO_DIRECTIONS(TO_ENUM)
} GPIO_DIRECTION;

static const char* GPIO_DIRECTION_STRINGS[] = {ALL_GPIO_DIRECTIONS(TO_STRING)};

STATUS gpio_export(int gpio, int* fd);
STATUS gpio_unexport(int gpio);
STATUS gpio_get_value(int fd, int* value);
STATUS gpio_set_value(int fd, int value);
STATUS gpio_set_edge(int gpio, GPIO_EDGE edge);
STATUS gpio_set_direction(int gpio, GPIO_DIRECTION direction);
STATUS gpio_set_active_low(int gpio, bool active_low);

#endif
