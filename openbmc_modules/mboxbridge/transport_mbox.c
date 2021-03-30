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
#include "transport_mbox.h"
#include "windows.h"
#include "lpc.h"

struct errno_map {
	int rc;
	int mbox_errno;
};

static const struct errno_map errno_map_v1[] = {
	{ 0, MBOX_R_SUCCESS },
	{ EACCES, MBOX_R_PARAM_ERROR },
	{ EBADMSG, MBOX_R_PARAM_ERROR },
	{ EBUSY, MBOX_R_SYSTEM_ERROR },
	{ EINVAL, MBOX_R_PARAM_ERROR },
	{ ENOTSUP, MBOX_R_PARAM_ERROR },
	{ EPERM, MBOX_R_PARAM_ERROR },
	{ EPROTO, MBOX_R_PARAM_ERROR },
	{ ETIMEDOUT, MBOX_R_TIMEOUT },
	{ -1, MBOX_R_SYSTEM_ERROR },
};

static const struct errno_map errno_map_v2[] = {
	{ 0, MBOX_R_SUCCESS },
	{ EACCES, MBOX_R_WINDOW_ERROR },
	{ EBADMSG, MBOX_R_SEQ_ERROR },
	{ EBUSY, MBOX_R_BUSY },
	{ EINVAL, MBOX_R_PARAM_ERROR },
	{ ENOTSUP, MBOX_R_PARAM_ERROR },
	{ EPERM, MBOX_R_WINDOW_ERROR },
	{ EPROTO, MBOX_R_PARAM_ERROR },
	{ ETIMEDOUT, MBOX_R_TIMEOUT },
	{ -1, MBOX_R_SYSTEM_ERROR },
};

static const struct errno_map *errno_maps[] = {
	[0] = NULL,
	[1] = errno_map_v1,
	[2] = errno_map_v2,
};

static inline int mbox_xlate_errno(struct mbox_context *context,
					     int rc)
{
	const struct errno_map *entry;

	rc = -rc;
	MSG_DBG("Translating errno %d: %s\n", rc, strerror(rc));
	for(entry = errno_maps[context->version]; entry->rc != -1; entry++) {
		if (rc == entry->rc) {
			return entry->mbox_errno;
		}
	}

	return entry->mbox_errno;
}

/*
 * transport_mbox_flush_events() - Write to the BMC controlled status register
 * 				   (reg 15)
 * @context:	The mbox context pointer
 *
 * Return:	0 on success otherwise negative error code
 */
static int transport_mbox_flush_events(struct mbox_context *context, uint8_t events)
{
	int rc;

	/* Seek mbox registers */
	rc = lseek(context->fds[MBOX_FD].fd, MBOX_BMC_EVENT, SEEK_SET);
	if (rc != MBOX_BMC_EVENT) {
		MSG_ERR("Couldn't lseek mbox to byte %d: %s\n", MBOX_BMC_EVENT,
				strerror(errno));
		return -errno;
	}

	/* Write to mbox status register */
	rc = write(context->fds[MBOX_FD].fd, &events, 1);
	if (rc != 1) {
		MSG_ERR("Couldn't write to BMC status reg: %s\n",
				strerror(errno));
		return -errno;
	}

	/* Reset to start */
	rc = lseek(context->fds[MBOX_FD].fd, 0, SEEK_SET);
	if (rc) {
		MSG_ERR("Couldn't reset MBOX offset to zero: %s\n",
				strerror(errno));
		return -errno;
	}

	return 0;
}

static int transport_mbox_put_events(struct mbox_context *context,
					uint8_t mask)
{
	return transport_mbox_flush_events(context, context->bmc_events & mask);
}

static int transport_mbox_update_events(struct mbox_context *context,
					uint8_t events, uint8_t mask)
{
	return transport_mbox_flush_events(context, context->bmc_events & mask);
}

static const struct transport_ops transport_mbox_ops = {
	.put_events = transport_mbox_put_events,
	.set_events = transport_mbox_update_events,
	.clear_events = transport_mbox_update_events,
};

/* Command Handlers */

/*
 * Command: RESET_STATE
 * Reset the LPC mapping to point back at the flash, or memory in case we're
 * using a virtual pnor.
 */
static int mbox_handle_reset(struct mbox_context *context,
			     union mbox_regs *req, struct mbox_msg *resp)
{
	return context->protocol->reset(context);
}

/*
 * Command: GET_MBOX_INFO
 * Get the API version, default window size and block size
 * We also set the LPC mapping to point to the reserved memory region here so
 * this command must be called before any window manipulation
 *
 * V1:
 * ARGS[0]: API Version
 *
 * RESP[0]: API Version
 * RESP[1:2]: Default read window size (number of blocks)
 * RESP[3:4]: Default write window size (number of blocks)
 * RESP[5]: Block size (as shift)
 *
 * V2:
 * ARGS[0]: API Version
 *
 * RESP[0]: API Version
 * RESP[1:2]: Default read window size (number of blocks)
 * RESP[3:4]: Default write window size (number of blocks)
 * RESP[5]: Block size (as shift)
 */
static int mbox_handle_mbox_info(struct mbox_context *context,
				 union mbox_regs *req, struct mbox_msg *resp)
{
	uint8_t mbox_api_version = req->msg.args[0];
	struct protocol_get_info io = {
		.req = { .api_version = mbox_api_version }
	};
	int rc;

	rc = context->protocol->get_info(context, &io);
	if (rc < 0) {
		return rc;
	}

	/*
	 * Switch transport to mbox, however we need to delay flushing the
	 * event state until after the command is processed.
	 */
	context->transport = &transport_mbox_ops;

	resp->args[0] = io.resp.api_version;
	if (io.resp.api_version == API_VERSION_1) {
		put_u16(&resp->args[1], io.resp.v1.read_window_size);
		put_u16(&resp->args[3], io.resp.v1.write_window_size);
	} else if (io.resp.api_version >= API_VERSION_2) {
		resp->args[5] = io.resp.v2.block_size_shift;
		put_u16(&resp->args[6], io.resp.v2.timeout);
	}

	return 0;
}

/*
 * Command: GET_FLASH_INFO
 * Get the flash size and erase granularity
 *
 * V1:
 * RESP[0:3]: Flash Size (bytes)
 * RESP[4:7]: Erase Size (bytes)
 * V2:
 * RESP[0:1]: Flash Size (number of blocks)
 * RESP[2:3]: Erase Size (number of blocks)
 */
static int mbox_handle_flash_info(struct mbox_context *context,
				  union mbox_regs *req, struct mbox_msg *resp)
{
	struct protocol_get_flash_info io;
	int rc;

	rc = context->protocol->get_flash_info(context, &io);
	if (rc < 0) {
		return rc;
	}

	switch (context->version) {
	case API_VERSION_1:
		/* Both Sizes in Bytes */
		put_u32(&resp->args[0], io.resp.v1.flash_size);
		put_u32(&resp->args[4], io.resp.v1.erase_size);
		break;
	case API_VERSION_2:
		/* Both Sizes in Block Size */
		put_u16(&resp->args[0], io.resp.v2.flash_size);
		put_u16(&resp->args[2], io.resp.v2.erase_size);
		break;
	default:
		MSG_ERR("API Version Not Valid - Invalid System State\n");
		return -MBOX_R_SYSTEM_ERROR;
	}

	return 0;
}

/*
 * get_lpc_addr_shifted() - Get lpc address of the current window
 * @context:		The mbox context pointer
 *
 * Return:	The lpc address to access that offset shifted by block size
 */
static inline uint16_t get_lpc_addr_shifted(struct mbox_context *context)
{
	uint32_t lpc_addr, mem_offset;

	/* Offset of the current window in the reserved memory region */
	mem_offset = context->current->mem - context->mem;
	/* Total LPC Address */
	lpc_addr = context->lpc_base + mem_offset;

	MSG_DBG("LPC address of current window: 0x%.8x\n", lpc_addr);

	return lpc_addr >> context->backend.block_size_shift;
}

static int mbox_handle_create_window(struct mbox_context *context, bool ro,
			      union mbox_regs *req, struct mbox_msg *resp)
{
	struct protocol_create_window io;
	int rc;

	io.req.offset = get_u16(&req->msg.args[0]);
	io.req.ro = ro;

	rc = context->protocol->create_window(context, &io);
	if (rc < 0) {
		return rc;
	}

	put_u16(&resp->args[0], io.resp.lpc_address);
	if (context->version >= API_VERSION_2) {
		put_u16(&resp->args[2], io.resp.size);
		put_u16(&resp->args[4], io.resp.offset);
	}

	return 0;
}

/*
 * Command: CREATE_READ_WINDOW
 * Opens a read window
 * First checks if any current window with the requested data, if so we just
 * point the host to that. Otherwise we read the request data in from flash and
 * point the host there.
 *
 * V1:
 * ARGS[0:1]: Window Location as Offset into Flash (number of blocks)
 *
 * RESP[0:1]: LPC bus address for host to access this window (number of blocks)
 *
 * V2:
 * ARGS[0:1]: Window Location as Offset into Flash (number of blocks)
 * ARGS[2:3]: Requested window size (number of blocks)
 *
 * RESP[0:1]: LPC bus address for host to access this window (number of blocks)
 * RESP[2:3]: Actual window size that the host can access (number of blocks)
 */
static int mbox_handle_read_window(struct mbox_context *context,
				   union mbox_regs *req, struct mbox_msg *resp)
{
	return mbox_handle_create_window(context, true, req, resp);
}

/*
 * Command: CREATE_WRITE_WINDOW
 * Opens a write window
 * First checks if any current window with the requested data, if so we just
 * point the host to that. Otherwise we read the request data in from flash and
 * point the host there.
 *
 * V1:
 * ARGS[0:1]: Window Location as Offset into Flash (number of blocks)
 *
 * RESP[0:1]: LPC bus address for host to access this window (number of blocks)
 *
 * V2:
 * ARGS[0:1]: Window Location as Offset into Flash (number of blocks)
 * ARGS[2:3]: Requested window size (number of blocks)
 *
 * RESP[0:1]: LPC bus address for host to access this window (number of blocks)
 * RESP[2:3]: Actual window size that was mapped/host can access (n.o. blocks)
 */
static int mbox_handle_write_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp)
{
	return mbox_handle_create_window(context, false, req, resp);
}

/*
 * Commands: MARK_WRITE_DIRTY
 * Marks a portion of the current (write) window dirty, informing the daemon
 * that is has been written to and thus must be at some point written to the
 * backing store
 * These changes aren't written back to the backing store unless flush is then
 * called or the window closed
 *
 * V1:
 * ARGS[0:1]: Where within flash to start (number of blocks)
 * ARGS[2:5]: Number to mark dirty (number of bytes)
 *
 * V2:
 * ARGS[0:1]: Where within window to start (number of blocks)
 * ARGS[2:3]: Number to mark dirty (number of blocks)
 */
static int mbox_handle_dirty_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp)
{
	struct protocol_mark_dirty io;

	if (context->version == API_VERSION_1) {
		io.req.v1.offset = get_u16(&req->msg.args[0]);
		io.req.v1.size = get_u32(&req->msg.args[2]);
	} else {
		io.req.v2.offset = get_u16(&req->msg.args[0]);
		io.req.v2.size = get_u16(&req->msg.args[2]);
	}

	return context->protocol->mark_dirty(context, &io);
}

/*
 * Commands: MARK_WRITE_ERASE
 * Erases a portion of the current window
 * These changes aren't written back to the backing store unless flush is then
 * called or the window closed
 *
 * V1:
 * Unimplemented
 *
 * V2:
 * ARGS[0:1]: Where within window to start (number of blocks)
 * ARGS[2:3]: Number to erase (number of blocks)
 */
static int mbox_handle_erase_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp)
{
	struct protocol_erase io;

	io.req.offset = get_u16(&req->msg.args[0]);
	io.req.size = get_u16(&req->msg.args[2]);

	if (!context->protocol->erase) {
		MSG_ERR("Protocol Version invalid for Erase Command\n");
		return -ENOTSUP;
	}

	return context->protocol->erase(context, &io);
}

/*
 * Command: WRITE_FLUSH
 * Flushes any dirty or erased blocks in the current window back to the backing
 * store
 * NOTE: For V1 this behaves much the same as the dirty command in that it
 * takes an offset and number of blocks to dirty, then also performs a flush as
 * part of the same command. For V2 this will only flush blocks already marked
 * dirty/erased with the appropriate commands and doesn't take any arguments
 * directly.
 *
 * V1:
 * ARGS[0:1]: Where within window to start (number of blocks)
 * ARGS[2:5]: Number to mark dirty (number of bytes)
 *
 * V2:
 * NONE
 */
static int mbox_handle_flush_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp)
{
	struct protocol_flush io = { 0 };

	if (context->version == API_VERSION_1) {
		io.req.offset = get_u16(&req->msg.args[0]);
		io.req.size = get_u32(&req->msg.args[2]);
	}

	return context->protocol->flush(context, &io);
}

/*
 * Command: CLOSE_WINDOW
 * Close the current window
 * NOTE: There is an implicit flush
 *
 * V1:
 * NONE
 *
 * V2:
 * ARGS[0]: FLAGS
 */
static int mbox_handle_close_window(struct mbox_context *context,
				    union mbox_regs *req, struct mbox_msg *resp)
{
	struct protocol_close io = { 0 };

	if (context->version >= API_VERSION_2) {
		io.req.flags = req->msg.args[0];
	}

	return context->protocol->close(context, &io);
}

/*
 * Command: BMC_EVENT_ACK
 * Sent by the host to acknowledge BMC events supplied in mailbox register 15
 *
 * ARGS[0]: Bitmap of bits to ack (by clearing)
 */
static int mbox_handle_ack(struct mbox_context *context, union mbox_regs *req,
			   struct mbox_msg *resp)
{
	struct protocol_ack io;

	io.req.flags = req->msg.args[0];

	return context->protocol->ack(context, &io);
}

/*
 * check_req_valid() - Check if the given request is a valid mbox request
 * @context:	The mbox context pointer
 * @cmd:	The request registers
 *
 * Return:	0 if request is valid otherwise negative error code
 */
static int check_req_valid(struct mbox_context *context, union mbox_regs *req)
{
	uint8_t cmd = req->msg.command;
	uint8_t seq = req->msg.seq;

	if (cmd > NUM_MBOX_CMDS) {
		MSG_ERR("Unknown mbox command: %d\n", cmd);
		return -ENOTSUP;
	}

	if (seq == context->prev_seq && cmd != MBOX_C_GET_MBOX_INFO) {
		MSG_ERR("Invalid sequence number: %d, previous: %d\n", seq,
			context->prev_seq);
		return -EBADMSG;
	}

	if (context->state & STATE_SUSPENDED) {
		if (cmd != MBOX_C_GET_MBOX_INFO && cmd != MBOX_C_ACK) {
			MSG_ERR("Cannot use that cmd while suspended: %d\n",
				cmd);
			return -EBUSY;
		}
	}

	if (context->transport != &transport_mbox_ops) {
		if (cmd != MBOX_C_RESET_STATE && cmd != MBOX_C_GET_MBOX_INFO) {
			MSG_ERR("Cannot switch transport with command %d\n",
				cmd);
			return -EPROTO;
		}
	}

	if (!(context->state & MAPS_MEM)) {
		if (cmd != MBOX_C_RESET_STATE && cmd != MBOX_C_GET_MBOX_INFO
					      && cmd != MBOX_C_ACK) {
			MSG_ERR("Must call GET_MBOX_INFO before %d\n", cmd);
			return -EPROTO;
		}
	}

	return 0;
}

typedef int (*mboxd_mbox_handler)(struct mbox_context *, union mbox_regs *,
				  struct mbox_msg *);

static const mboxd_mbox_handler transport_mbox_handlers[] = {
	mbox_handle_reset,
	mbox_handle_mbox_info,
	mbox_handle_flash_info,
	mbox_handle_read_window,
	mbox_handle_close_window,
	mbox_handle_write_window,
	mbox_handle_dirty_window,
	mbox_handle_flush_window,
	mbox_handle_ack,
	mbox_handle_erase_window
};

/*
 * handle_mbox_req() - Handle an incoming mbox command request
 * @context:	The mbox context pointer
 * @req:	The mbox request message
 *
 * Return:	0 if handled successfully otherwise negative error code
 */
static int handle_mbox_req(struct mbox_context *context, union mbox_regs *req)
{
	const struct transport_ops *old_transport = context->transport;
	struct mbox_msg resp = {
		.command = req->msg.command,
		.seq = req->msg.seq,
		.args = { 0 },
		.response = MBOX_R_SUCCESS
	};
	int rc = 0, len, i;

	MSG_INFO("Received MBOX command: %u\n", req->msg.command);

	rc = check_req_valid(context, req);
	if (!rc) {
		mboxd_mbox_handler handler;

		/* Commands start at 1 so we have to subtract 1 from the cmd */
		handler = transport_mbox_handlers[req->msg.command - 1];
		rc = handler(context, req, &resp);
		if (rc < 0) {
			MSG_ERR("Error handling mbox cmd: %d\n",
				req->msg.command);
		}
	}

	rc = mbox_xlate_errno(context, rc);
	resp.response = rc;
	context->prev_seq = req->msg.seq;

	MSG_DBG("Writing MBOX response:\n");
	MSG_DBG("MBOX cmd: %u\n", resp.command);
	MSG_DBG("MBOX seq: %u\n", resp.seq);
	for (i = 0; i < MBOX_ARGS_BYTES; i++) {
		MSG_DBG("MBOX arg[%d]: 0x%.2x\n", i, resp.args[i]);
	}
	MSG_INFO("Writing MBOX response: %u\n", resp.response);
	len = write(context->fds[MBOX_FD].fd, &resp, sizeof(resp));
	if (len < sizeof(resp)) {
		MSG_ERR("Didn't write the full response\n");
		rc = -errno;
	}

	if (context->transport != old_transport &&
			context->transport == &transport_mbox_ops) {
		/* A bit messy, but we need the correct event mask */
		protocol_events_set(context, context->bmc_events);
	}

	return rc;
}

/*
 * get_message() - Read an mbox request message from the mbox registers
 * @context:	The mbox context pointer
 * @msg:	Where to put the received message
 *
 * Return:	0 if read successfully otherwise negative error code
 */
static int get_message(struct mbox_context *context, union mbox_regs *msg)
{
	int rc, i;

	rc = read(context->fds[MBOX_FD].fd, msg, sizeof(msg->raw));
	if (rc < 0) {
		MSG_ERR("Couldn't read: %s\n", strerror(errno));
		return -errno;
	} else if (rc < sizeof(msg->raw)) {
		MSG_ERR("Short read: %d expecting %zu\n", rc, sizeof(msg->raw));
		return -1;
	}

	MSG_DBG("Received MBOX request:\n");
	MSG_DBG("MBOX cmd: %u\n", msg->msg.command);
	MSG_DBG("MBOX seq: %u\n", msg->msg.seq);
	for (i = 0; i < MBOX_ARGS_BYTES; i++) {
		MSG_DBG("MBOX arg[%d]: 0x%.2x\n", i, msg->msg.args[i]);
	}

	return 0;
}

/*
 * transport_mbox_dispatch() - handle an mbox interrupt
 * @context:	The mbox context pointer
 *
 * Return:	0 if handled successfully otherwise negative error code
 */
int transport_mbox_dispatch(struct mbox_context *context)
{
	int rc = 0;
	union mbox_regs req = { 0 };

	assert(context);

	rc = get_message(context, &req);
	if (rc) {
		return rc;
	}

	return handle_mbox_req(context, &req);
}

int __transport_mbox_init(struct mbox_context *context, const char *path,
			  const struct transport_ops **ops)
{
	int fd;

	/* Open MBOX Device */
	fd = open(path, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		MSG_INFO("Couldn't open %s with flags O_RDWR: %s\n",
			path, strerror(errno));
		return -errno;
	}
	MSG_DBG("Opened mbox dev: %s\n", path);

	context->fds[MBOX_FD].fd = fd;

	if (ops) {
		*ops = &transport_mbox_ops;
	}

	return 0;
}

int transport_mbox_init(struct mbox_context *context,
			const struct transport_ops **ops)
{
	int rc;

	rc = __transport_mbox_init(context, MBOX_HOST_PATH, ops);
	if (rc)
		return rc;

	return 0;
}

void transport_mbox_free(struct mbox_context *context)
{
	close(context->fds[MBOX_FD].fd);
}
