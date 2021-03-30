/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef MBOXD_FLASH_H
#define MBOXD_FLASH_H

#define FLASH_DIRTY	0x00
#define FLASH_ERASED	0x01

#include "mbox.h"

#ifdef __cplusplus
extern "C" {
#endif

int init_flash_dev(struct mbox_context *context);
void free_flash_dev(struct mbox_context *context);
int64_t copy_flash(struct mbox_context *context, uint32_t offset, void *mem,
		   uint32_t size);
int set_flash_bytemap(struct mbox_context *context, uint32_t offset,
		      uint32_t count, uint8_t val);
int erase_flash(struct mbox_context *context, uint32_t offset, uint32_t count);
int write_flash(struct mbox_context *context, uint32_t offset, void *buf,
		uint32_t count);

#ifdef __cplusplus
}
#endif
#endif /* MBOXD_FLASH_H */
