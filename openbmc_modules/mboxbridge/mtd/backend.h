/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2019 IBM Corp. */

#ifndef MTD_BACKEND_H
#define MTD_BACKEND_H

struct mtd_data {
	int fd;
	uint8_t *flash_bmap;
	struct mtd_info_user mtd_info;
};

#endif
