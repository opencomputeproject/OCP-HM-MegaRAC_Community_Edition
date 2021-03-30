/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef MBOXD_MSG_H
#define MBOXD_MSG_H

#include <stdint.h>

struct mbox_context;
struct transport_ops;

/* Command Values */
#define MBOX_C_RESET_STATE		0x01
#define MBOX_C_GET_MBOX_INFO		0x02
#define MBOX_C_GET_FLASH_INFO		0x03
#define MBOX_C_READ_WINDOW		0x04
#define MBOX_C_CLOSE_WINDOW		0x05
#define MBOX_C_WRITE_WINDOW		0x06
#define MBOX_C_WRITE_DIRTY		0x07
#define MBOX_C_WRITE_FLUSH		0x08
#define MBOX_C_ACK			0x09
#define MBOX_C_WRITE_ERASE		0x0a
#define NUM_MBOX_CMDS			MBOX_C_WRITE_ERASE

/* Response Values */
#define MBOX_R_SUCCESS			0x01
#define MBOX_R_PARAM_ERROR		0x02
#define MBOX_R_WRITE_ERROR		0x03
#define MBOX_R_SYSTEM_ERROR		0x04
#define MBOX_R_TIMEOUT			0x05
#define MBOX_R_BUSY			0x06
#define MBOX_R_WINDOW_ERROR		0x07
#define MBOX_R_SEQ_ERROR		0x08

/* MBOX Registers */
#define MBOX_HOST_PATH			"/dev/aspeed-mbox"
#define MBOX_HOST_TIMEOUT_SEC		1
#define MBOX_ARGS_BYTES			11
#define MBOX_REG_BYTES			16
#define MBOX_HOST_EVENT			14
#define MBOX_BMC_EVENT			15

struct mbox_msg {
	uint8_t command;
	uint8_t seq;
	uint8_t args[MBOX_ARGS_BYTES];
	uint8_t response;
};

union mbox_regs {
	uint8_t raw[MBOX_REG_BYTES];
	struct mbox_msg msg;
};

int transport_mbox_dispatch(struct mbox_context *context);
int transport_mbox_init(struct mbox_context *context,
			const struct transport_ops **ops);
void transport_mbox_free(struct mbox_context *context);

#endif /* MBOXD_MSG_H */
