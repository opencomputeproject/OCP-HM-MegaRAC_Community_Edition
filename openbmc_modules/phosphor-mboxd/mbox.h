/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef MBOX_H
#define MBOX_H

#include <mtd/mtd-abi.h>
#include <systemd/sd-bus.h>
#include <poll.h>
#include <stdbool.h>
#include "vpnor/mboxd_pnor_partition_table.h"

enum api_version {
	API_VERSION_INVAL	= 0,
	API_VERSION_1		= 1,
	API_VERSION_2		= 2
};

#define API_MIN_VERSION			API_VERSION_1
#define API_MAX_VERSION			API_VERSION_2

#define THIS_NAME			"Mailbox Daemon"

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

/* Argument Flags */
#define FLAGS_NONE			0x00
#define FLAGS_SHORT_LIFETIME		0x01

/* BMC Event Notification */
#define BMC_EVENT_REBOOT		0x01
#define BMC_EVENT_WINDOW_RESET		0x02
#define BMC_EVENT_ACK_MASK		(BMC_EVENT_REBOOT | \
					BMC_EVENT_WINDOW_RESET)
#define BMC_EVENT_FLASH_CTRL_LOST	0x40
#define BMC_EVENT_DAEMON_READY		0x80
#define BMC_EVENT_V1_MASK		BMC_EVENT_REBOOT
#define BMC_EVENT_V2_MASK		(BMC_EVENT_REBOOT | \
					BMC_EVENT_WINDOW_RESET | \
					BMC_EVENT_FLASH_CTRL_LOST | \
					BMC_EVENT_DAEMON_READY)

/* MBOX Registers */
#define MBOX_HOST_PATH			"/dev/aspeed-mbox"
#define MBOX_HOST_TIMEOUT_SEC		1
#define MBOX_ARGS_BYTES			11
#define MBOX_REG_BYTES			16
#define MBOX_HOST_EVENT			14
#define MBOX_BMC_EVENT			15

#define BLOCK_SIZE_SHIFT_V1		12 /* 4K */

/* Window Dirty/Erase bytemap masks */
#define WINDOW_CLEAN			0x00
#define WINDOW_DIRTY			0x01
#define WINDOW_ERASED			0x02

/* Put polled file descriptors first */
#define DBUS_FD			0
#define MBOX_FD			1
#define SIG_FD			2
#define POLL_FDS		3 /* Number of FDs we poll on */
#define LPC_CTRL_FD		3
#define MTD_FD			4
#define TOTAL_FDS		5

#define MAPS_FLASH		(1 << 0)
#define MAPS_MEM		(1 << 1)
#define STATE_SUSPENDED		(1 << 7)
enum mbox_state {
	/* Still Initing */
	UNINITIALISED = 0,
	/* Active and LPC Maps Flash */
	ACTIVE_MAPS_FLASH = MAPS_FLASH,
	/* Suspended and LPC Maps Flash */
	SUSPEND_MAPS_FLASH = STATE_SUSPENDED | MAPS_FLASH,
	/* Active and LPC Maps Memory */
	ACTIVE_MAPS_MEM = MAPS_MEM,
	/* Suspended and LPC Maps Memory */
	SUSPEND_MAPS_MEM = STATE_SUSPENDED | MAPS_MEM
};

#define FLASH_OFFSET_UNINIT	0xFFFFFFFF

struct window_context {
	void *mem;			/* Portion of Reserved Memory Region */
	uint32_t flash_offset;		/* Flash area the window maps (bytes) */
	uint32_t size;			/* Window Size (bytes) power-of-2 */
	uint8_t *dirty_bmap;		/* Bytemap of the dirty/erased state */
	uint32_t age;			/* Used for LRU eviction scheme */
};

struct window_list {
	uint32_t num;
	uint32_t max_age;
	uint32_t default_size;
	struct window_context *window;
};

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

struct mbox_context;

typedef int (*mboxd_mbox_handler)(struct mbox_context *, union mbox_regs *,
				  struct mbox_msg *);

struct mbox_context {
/* System State */
	enum mbox_state state;
	enum api_version version;
	struct pollfd fds[TOTAL_FDS];
	sd_bus *bus;
	bool terminate;
	uint8_t bmc_events;
	uint8_t prev_seq;

/* Command Dispatch */
	const mboxd_mbox_handler *handlers;

/* Window State */
	/* The window list struct containing all current "windows" */
	struct window_list windows;
	/* The window the host is currently pointed at */
	struct window_context *current;
	/* Is the current window a write one */
	bool current_is_write;

/* Memory & Flash State */
	/* Reserved Memory Region */
	void *mem;
	/* Reserved Mem Size (bytes) */
	uint32_t mem_size;
	/* LPC Bus Base Address (bytes) */
	uint32_t lpc_base;
	/* Flash size from command line (bytes) */
	uint32_t flash_size;
	/* Bytemap of the erased state of the entire flash */
	uint8_t *flash_bmap;
	/* Erase size (as a shift) */
	uint32_t erase_size_shift;
	/* Block size (as a shift) */
	uint32_t block_size_shift;
	/* Actual Flash Info */
	struct mtd_info_user mtd_info;
#ifdef VIRTUAL_PNOR_ENABLED
	/* Virtual PNOR partition table */
	struct vpnor_partition_table *vpnor;
	struct vpnor_partition_paths paths;
#endif
};

#endif /* MBOX_H */
