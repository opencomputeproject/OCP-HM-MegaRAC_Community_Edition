// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#include "mboxd.h"
#include "common.h"
#include "lpc.h"
#include "backend.h"
#include <linux/aspeed-lpc-ctrl.h>

#define LPC_CTRL_PATH		"/dev/aspeed-lpc-ctrl"

int __lpc_dev_init(struct mbox_context *context, const char *path)
{
	struct aspeed_lpc_ctrl_mapping map = {
		.window_type = ASPEED_LPC_CTRL_WINDOW_MEMORY,
		.window_id = 0, /* There's only one */
		.flags = 0,
		.addr = 0,
		.offset = 0,
		.size = 0
	};
	int fd;

	/* Open LPC Device */
	MSG_DBG("Opening %s\n", path);
	fd = open(path, O_RDWR | O_SYNC);
	if (fd < 0) {
		MSG_ERR("Couldn't open %s with flags O_RDWR: %s\n",
			path, strerror(errno));
		return -errno;
	}

	context->fds[LPC_CTRL_FD].fd = fd;

	/* Find Size of Reserved Memory Region */
	MSG_DBG("Getting buffer size...\n");
	if (ioctl(fd, ASPEED_LPC_CTRL_IOCTL_GET_SIZE, &map) < 0) {
		MSG_ERR("Couldn't get lpc control buffer size: %s\n",
			strerror(errno));
		return -errno;
	}

	context->mem_size = map.size;
	/* Map at the top of the 28-bit LPC firmware address space-0 */
	context->lpc_base = 0x0FFFFFFF & -context->mem_size;

	/* mmap the Reserved Memory Region */
	MSG_DBG("Mapping in 0x%.8x bytes of %s\n", context->mem_size, path);
	context->mem = mmap(NULL, context->mem_size, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, 0);
	if (context->mem == MAP_FAILED) {
		MSG_ERR("Failed to map %s: %s\n", path, strerror(errno));
		return -errno;
	}

	return 0;
}

int lpc_dev_init(struct mbox_context *context)
{
	return __lpc_dev_init(context, LPC_CTRL_PATH);
}

void lpc_dev_free(struct mbox_context *context)
{
	if (context->mem) {
		munmap(context->mem, context->mem_size);
	}
	close(context->fds[LPC_CTRL_FD].fd);
}

/*
 * lpc_map_flash() - Point the lpc bus mapping to the actual flash device
 * @context:	The mbox context pointer
 *
 * Return:	0 on success otherwise negative error code
 */
int lpc_map_flash(struct mbox_context *context)
{
	struct aspeed_lpc_ctrl_mapping map = {
		.window_type = ASPEED_LPC_CTRL_WINDOW_FLASH,
		.window_id = 0, /* Theres only one */
		.flags = 0,
		/*
		 * The mask is because the top nibble is the host LPC FW space,
		 * we want space 0.
		 */
		.addr = 0x0FFFFFFF & -context->backend.flash_size,
		.offset = 0,
		.size = context->backend.flash_size
	};

	if (context->state & MAPS_FLASH) {
		return 0; /* LPC Bus already points to flash */
	}
	/* Don't let the host access flash while we're suspended */
	if (context->state & STATE_SUSPENDED) {
		MSG_ERR("Can't point lpc mapping to flash while suspended\n");
		return -EBUSY;
	}

	MSG_INFO("Pointing HOST LPC bus at the flash\n");
	MSG_INFO("Assuming %dMB of flash: HOST LPC 0x%08x\n",
		context->backend.flash_size >> 20, map.addr);

	if (ioctl(context->fds[LPC_CTRL_FD].fd, ASPEED_LPC_CTRL_IOCTL_MAP, &map)
			== -1) {
		MSG_ERR("Failed to point the LPC BUS at the actual flash: %s\n",
			strerror(errno));
		return -errno;
	}

	context->state = ACTIVE_MAPS_FLASH;
	/*
	 * Since the host now has access to the flash it can change it out from
	 * under us
	 */
	return backend_set_bytemap(&context->backend, 0,
				   context->backend.flash_size, FLASH_DIRTY);
}

/*
 * lpc_map_memory() - Point the lpc bus mapping to the reserved memory region
 * @context:	The mbox context pointer
 *
 * Return:	0 on success otherwise negative error code
 */
int lpc_map_memory(struct mbox_context *context)
{
	struct aspeed_lpc_ctrl_mapping map = {
		.window_type = ASPEED_LPC_CTRL_WINDOW_MEMORY,
		.window_id = 0, /* There's only one */
		.flags = 0,
		.addr = context->lpc_base,
		.offset = 0,
		.size = context->mem_size
	};

	if (context->state & MAPS_MEM) {
		return 0; /* LPC Bus already points to reserved memory area */
	}

	MSG_INFO("Pointing HOST LPC bus at memory region %p of size 0x%.8x\n",
			context->mem, context->mem_size);
	MSG_INFO("LPC address 0x%.8x\n", map.addr);

	if (ioctl(context->fds[LPC_CTRL_FD].fd, ASPEED_LPC_CTRL_IOCTL_MAP,
		  &map)) {
		MSG_ERR("Failed to point the LPC BUS to memory: %s\n",
			strerror(errno));
		return -errno;
	}

	/* LPC now maps memory (keep suspended state) */
	context->state = MAPS_MEM | (context->state & STATE_SUSPENDED);

	return 0;
}
