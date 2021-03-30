/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef TEST_SYSTEM_H
#define TEST_SYSTEM_H

#include <stdint.h>

void system_set_reserved_size(uint32_t size);
void system_set_mtd_sizes(uint32_t size, uint32_t erasesize);

#endif /* TEST_SYSTEM_H */
