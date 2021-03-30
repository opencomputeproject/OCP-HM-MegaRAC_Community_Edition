/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef MBOXD_MSG_H
#define MBOXD_MSG_H

#include "common.h"
#include "mbox.h"

/* Estimate as to how long (milliseconds) it takes to access a MB from flash */
#define FLASH_ACCESS_MS_PER_MB		8000

#define NO_BMC_EVENT			false
#define SET_BMC_EVENT			true

int set_bmc_events(struct mbox_context *context, uint8_t bmc_event,
		   bool write_back);
int clr_bmc_events(struct mbox_context *context, uint8_t bmc_event,
		   bool write_back);
int dispatch_mbox(struct mbox_context *context);
int init_mbox_dev(struct mbox_context *context);
void free_mbox_dev(struct mbox_context *context);

/* Command handlers */
int mbox_handle_reset(struct mbox_context *context,
			     union mbox_regs *req, struct mbox_msg *resp);
int mbox_handle_mbox_info(struct mbox_context *context,
				 union mbox_regs *req, struct mbox_msg *resp);
int mbox_handle_flash_info(struct mbox_context *context,
				  union mbox_regs *req, struct mbox_msg *resp);
int mbox_handle_read_window(struct mbox_context *context,
				   union mbox_regs *req, struct mbox_msg *resp);
int mbox_handle_close_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp);
int mbox_handle_write_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp);
int mbox_handle_dirty_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp);
int mbox_handle_flush_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp);
int mbox_handle_ack(struct mbox_context *context, union mbox_regs *req,
			   struct mbox_msg *resp);
int mbox_handle_erase_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp);

#endif /* MBOXD_MSG_H */
