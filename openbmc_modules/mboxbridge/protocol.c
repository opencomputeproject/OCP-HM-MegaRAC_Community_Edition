// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include "config.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "backend.h"
#include "common.h"
#include "lpc.h"
#include "mboxd.h"
#include "protocol.h"
#include "windows.h"


#define BLOCK_SIZE_SHIFT_V1		12 /* 4K */

static inline uint8_t protocol_get_bmc_event_mask(struct mbox_context *context)
{
	if (context->version == API_VERSION_1) {
		return BMC_EVENT_V1_MASK;
	}

	return BMC_EVENT_V2_MASK;
}

/*
 * protocol_events_put() - Push the full set/cleared state of BMC events on the
 * 			   provided transport
 * @context:    The mbox context pointer
 * @ops:	The operations struct for the transport of interest
 *
 * Return:      0 on success otherwise negative error code
 */
int protocol_events_put(struct mbox_context *context,
			const struct transport_ops *ops)
{
	const uint8_t mask = protocol_get_bmc_event_mask(context);

	return ops->put_events(context, mask);
}

/*
 * protocol_events_set() - Update the set BMC events on the active transport
 * @context:	The mbox context pointer
 * @bmc_event:	The bits to set
 *
 * Return:	0 on success otherwise negative error code
 */
int protocol_events_set(struct mbox_context *context, uint8_t bmc_event)
{
	const uint8_t mask = protocol_get_bmc_event_mask(context);

	/*
	 * Store the raw value, as we may up- or down- grade the protocol
	 * version and subsequently need to flush the appropriate set. Instead
	 * we pass the masked value through to the transport
	 */
	context->bmc_events |= bmc_event;

	return context->transport->set_events(context, bmc_event, mask);
}

/*
 * protocol_events_clear() - Update the cleared BMC events on the active
 *                           transport
 * @context:	The mbox context pointer
 * @bmc_event:	The bits to clear
 *
 * Return:	0 on success otherwise negative error code
 */
int protocol_events_clear(struct mbox_context *context, uint8_t bmc_event)
{
	const uint8_t mask = protocol_get_bmc_event_mask(context);

	context->bmc_events &= ~bmc_event;

	return context->transport->clear_events(context, bmc_event, mask);
}

static int protocol_negotiate_version(struct mbox_context *context,
				      uint8_t requested);

static int protocol_v1_reset(struct mbox_context *context)
{
	return __protocol_reset(context);
}

static int protocol_negotiate_version(struct mbox_context *context,
				      uint8_t requested);

static int protocol_v1_get_info(struct mbox_context *context,
				struct protocol_get_info *io)
{
	uint8_t old_version = context->version;
	int rc;

	/* Bootstrap protocol version. This may involve {up,down}grading */
	rc = protocol_negotiate_version(context, io->req.api_version);
	if (rc < 0)
		return rc;

	/* Do the {up,down}grade if necessary*/
	if (rc != old_version) {
		/* Doing version negotiation, don't alert host to reset */
		windows_reset_all(context);
		return context->protocol->get_info(context, io);
	}

	/* Record the negotiated version for the response */
	io->resp.api_version = rc;

	/* Now do all required intialisation for v1 */
	context->backend.block_size_shift = BLOCK_SIZE_SHIFT_V1;
	MSG_INFO("Block Size: 0x%.8x (shift: %u)\n",
		 1 << context->backend.block_size_shift, context->backend.block_size_shift);

	/* Knowing blocksize we can allocate the window dirty_bytemap */
	windows_alloc_dirty_bytemap(context);

	io->resp.v1.read_window_size =
		context->windows.default_size >> context->backend.block_size_shift;
	io->resp.v1.write_window_size =
		context->windows.default_size >> context->backend.block_size_shift;

	return lpc_map_memory(context);
}

static int protocol_v1_get_flash_info(struct mbox_context *context,
				      struct protocol_get_flash_info *io)
{
	io->resp.v1.flash_size = context->backend.flash_size;
	io->resp.v1.erase_size = 1 << context->backend.erase_size_shift;

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

static inline int64_t blktrace_gettime(void)
{
	struct timespec ts;
	int64_t n;

	clock_gettime(CLOCK_REALTIME, &ts);
	n = (int64_t)(ts.tv_sec) * (int64_t)1000000000 + (int64_t)(ts.tv_nsec);

	return n;
}

static void blktrace_flush_start(struct mbox_context *context)
{
	struct blk_io_trace *trace = &context->trace;
	struct timespec now;

	if (!context->blktracefd)
		return;

	if (!context->blktrace_start) {
		clock_gettime(CLOCK_REALTIME, &now);
		context->blktrace_start = blktrace_gettime();
	}

	trace->magic = BLK_IO_TRACE_MAGIC | BLK_IO_TRACE_VERSION;
	trace->sequence++;
	trace->time = blktrace_gettime() - context->blktrace_start;
	trace->sector = context->current->flash_offset / 512;
	trace->bytes = context->current->size;
	if (context->current_is_write)
		trace->action = BLK_TA_QUEUE | BLK_TC_ACT(BLK_TC_WRITE);
	else
		trace->action = BLK_TA_QUEUE | BLK_TC_ACT(BLK_TC_READ);
	trace->pid = 0;
	trace->device = 0;
	trace->cpu = 0;
	trace->error = 0;
	trace->pdu_len = 0;
	write(context->blktracefd, trace, sizeof(*trace));
	trace->sequence++;
	trace->time = blktrace_gettime() - context->blktrace_start;
	trace->action &= ~BLK_TA_QUEUE;
	trace->action |= BLK_TA_ISSUE;
	write(context->blktracefd, trace, sizeof(*trace));
}

static void blktrace_flush_done(struct mbox_context *context)
{
	struct blk_io_trace *trace = &context->trace;

	if (!context->blktracefd)
		return;

	trace->sequence++;
	trace->time = blktrace_gettime() - context->blktrace_start;
	trace->action &= ~BLK_TA_ISSUE;
	trace->action |= BLK_TA_COMPLETE;
	write(context->blktracefd, trace, sizeof(*trace));
}

static void blktrace_window_start(struct mbox_context *context)
{
	struct blk_io_trace *trace = &context->trace;

	if (!context->blktracefd)
		return;

	if (!context->blktrace_start)
		context->blktrace_start = blktrace_gettime();

	trace->magic = BLK_IO_TRACE_MAGIC | BLK_IO_TRACE_VERSION;
	trace->sequence++;
	trace->time = blktrace_gettime() - context->blktrace_start;
	trace->action = BLK_TA_QUEUE | BLK_TC_ACT(BLK_TC_READ);
	trace->pid = 0;
	trace->device = 0;
	trace->cpu = 0;
	trace->error = 0;
	trace->pdu_len = 0;
}

static void blktrace_window_done(struct mbox_context *context)
{
	struct blk_io_trace *trace = &context->trace;

	if (!context->blktracefd)
		return;

	trace->sector = context->current->flash_offset / 512;
	trace->bytes = context->current->size;
	write(context->blktracefd, trace, sizeof(*trace));
	trace->sequence++;
	trace->action &= ~BLK_TA_QUEUE;
	trace->action |= BLK_TA_ISSUE;
	write(context->blktracefd, trace, sizeof(*trace));

	trace->sequence++;
	trace->time = blktrace_gettime() - context->blktrace_start;
	trace->action &= ~BLK_TA_ISSUE;
	trace->action |= BLK_TA_COMPLETE;
	write(context->blktracefd, trace, sizeof(*trace));
}

static int protocol_v1_create_window(struct mbox_context *context,
				     struct protocol_create_window *io)
{
	struct backend *backend = &context->backend;
	uint32_t offset;
	uint32_t size;
	int rc;

	offset = io->req.offset << backend->block_size_shift;
	size = io->req.size << backend->block_size_shift;
	rc = backend_validate(backend, offset, size, io->req.ro);
	if (rc < 0) {
		/* Backend does not allow window to be created. */
		return rc;
	}

	/* Close the current window if there is one */
	if (context->current) {
		/* There is an implicit flush if it was a write window
		 *
		 * protocol_v2_create_window() calls
		 * protocol_v1_create_window(), so use indirect call to
		 * write_flush() to make sure we pick the right one.
		 */
		if (context->current_is_write) {
			blktrace_flush_start(context);
			rc = context->protocol->flush(context, NULL);
			blktrace_flush_done(context);
			if (rc < 0) {
				MSG_ERR("Couldn't Flush Write Window\n");
				return rc;
			}
		}
		windows_close_current(context, FLAGS_NONE);
	}

	/* Offset the host has requested */
	MSG_INFO("Host requested flash @ 0x%.8x\n", offset);
	/* Check if we have an existing window */
	blktrace_window_start(context);
	context->current = windows_search(context, offset,
					  context->version == API_VERSION_1);

	if (!context->current) { /* No existing window */
		MSG_DBG("No existing window which maps that flash offset\n");
		rc = windows_create_map(context, &context->current,
				       offset,
				       context->version == API_VERSION_1);
		if (rc < 0) { /* Unable to map offset */
			MSG_ERR("Couldn't create window mapping for offset 0x%.8x\n",
				offset);
			return rc;
		}
	}
	blktrace_window_done(context);

	context->current_is_write = !io->req.ro;

	MSG_INFO("Window @ %p for size 0x%.8x maps flash offset 0x%.8x\n",
		 context->current->mem, context->current->size,
		 context->current->flash_offset);

	io->resp.lpc_address = get_lpc_addr_shifted(context);

	return 0;
}

static int protocol_v1_mark_dirty(struct mbox_context *context,
				  struct protocol_mark_dirty *io)
{
	uint32_t offset = io->req.v1.offset;
	uint32_t size = io->req.v1.size;
	uint32_t off;

	if (!(context->current && context->current_is_write)) {
		MSG_ERR("Tried to call mark dirty without open write window\n");
		return -EPERM;
	}

	/* For V1 offset given relative to flash - we want the window */
	off = offset - ((context->current->flash_offset) >>
			context->backend.block_size_shift);
	if (off > offset) { /* Underflow - before current window */
		MSG_ERR("Tried to mark dirty before start of window\n");
		MSG_ERR("requested offset: 0x%x window start: 0x%x\n",
				offset << context->backend.block_size_shift,
				context->current->flash_offset);
		return -EINVAL;
	}
	offset = off;
	/*
	 * We only track dirty at the block level.
	 * For protocol V1 we can get away with just marking the whole
	 * block dirty.
	 */
	size = align_up(size, 1 << context->backend.block_size_shift);
	size >>= context->backend.block_size_shift;

	MSG_INFO("Dirty window @ 0x%.8x for 0x%.8x\n",
		 offset << context->backend.block_size_shift,
		 size << context->backend.block_size_shift);

	return window_set_bytemap(context, context->current, offset, size,
				  WINDOW_DIRTY);
}

static int generic_flush(struct mbox_context *context)
{
	int rc, i, offset, count;
	uint8_t prev;

	offset = 0;
	count = 0;
	prev = WINDOW_CLEAN;

	MSG_INFO("Flush window @ %p for size 0x%.8x which maps flash @ 0x%.8x\n",
		 context->current->mem, context->current->size,
		 context->current->flash_offset);

	/*
	 * We look for streaks of the same type and keep a count, when the type
	 * (dirty/erased) changes we perform the required action on the backing
	 * store and update the current streak-type
	 */
	for (i = 0; i < (context->current->size >> context->backend.block_size_shift);
			i++) {
		uint8_t cur = context->current->dirty_bmap[i];
		if (cur != WINDOW_CLEAN) {
			if (cur == prev) { /* Same as previous block, incrmnt */
				count++;
			} else if (prev == WINDOW_CLEAN) { /* Start of run */
				offset = i;
				count++;
			} else { /* Change in streak type */
				rc = window_flush(context, offset, count,
						       prev);
				if (rc < 0) {
					return rc;
				}
				offset = i;
				count = 1;
			}
		} else {
			if (prev != WINDOW_CLEAN) { /* End of a streak */
				rc = window_flush(context, offset, count,
						       prev);
				if (rc < 0) {
					return rc;
				}
				offset = 0;
				count = 0;
			}
		}
		prev = cur;
	}

	if (prev != WINDOW_CLEAN) { /* Still the last streak to write */
		rc = window_flush(context, offset, count, prev);
		if (rc < 0) {
			return rc;
		}
	}

	/* Clear the dirty bytemap since we have written back all changes */
	return window_set_bytemap(context, context->current, 0,
				  context->current->size >>
				  context->backend.block_size_shift,
				  WINDOW_CLEAN);
}

static int protocol_v1_flush(struct mbox_context *context,
			     struct protocol_flush *io)
{
	int rc;

	if (!(context->current && context->current_is_write)) {
		MSG_ERR("Tried to call flush without open write window\n");
		return -EPERM;
	}

	/*
	 * For V1 the Flush command acts much the same as the dirty command
	 * except with a flush as well. Only do this on an actual flush
	 * command not when we call flush because we've implicitly closed a
	 * window because we might not have the required args in req.
	 */
	if (io) {
		struct protocol_mark_dirty *mdio = (void *)io;
		rc = protocol_v1_mark_dirty(context, mdio);
		if (rc < 0) {
			return rc;
		}
	}

	return generic_flush(context);
}

static int protocol_v1_close(struct mbox_context *context,
			     struct protocol_close *io)
{
	int rc;

	/* Close the current window if there is one */
	if (!context->current) {
		return 0;
	}

	/* There is an implicit flush if it was a write window */
	if (context->current_is_write) {
		rc = protocol_v1_flush(context, NULL);
		if (rc < 0) {
			MSG_ERR("Couldn't Flush Write Window\n");
			return rc;
		}
	}

	/* Host asked for it -> Don't set the BMC Event */
	windows_close_current(context, io->req.flags);

	return 0;
}

static int protocol_v1_ack(struct mbox_context *context,
			   struct protocol_ack *io)
{
	return protocol_events_clear(context,
				     (io->req.flags & BMC_EVENT_ACK_MASK));
}

/*
 * get_suggested_timeout() - get the suggested timeout value in seconds
 * @context:	The mbox context pointer
 *
 * Return:	Suggested timeout in seconds
 */
static uint16_t get_suggested_timeout(struct mbox_context *context)
{
	struct window_context *window = windows_find_largest(context);
	uint32_t max_size_mb = window ? (window->size >> 20) : 0;
	uint16_t ret;

	ret = align_up(max_size_mb * FLASH_ACCESS_MS_PER_MB, 1000) / 1000;

	MSG_DBG("Suggested Timeout: %us, max window size: %uMB, for %dms/MB\n",
		ret, max_size_mb, FLASH_ACCESS_MS_PER_MB);
	return ret;
}

static int protocol_v2_get_info(struct mbox_context *context,
				struct protocol_get_info *io)
{
	uint8_t old_version = context->version;
	int rc;

	/* Bootstrap protocol version. This may involve {up,down}grading */
	rc = protocol_negotiate_version(context, io->req.api_version);
	if (rc < 0)
		return rc;

	/* Do the {up,down}grade if necessary*/
	if (rc != old_version) {
		/* Doing version negotiation, don't alert host to reset */
		windows_reset_all(context);
		return context->protocol->get_info(context, io);
	}

	/* Record the negotiated version for the response */
	io->resp.api_version = rc;

	/* Now do all required intialisation for v2 */

	/* Knowing blocksize we can allocate the window dirty_bytemap */
	windows_alloc_dirty_bytemap(context);

	io->resp.v2.block_size_shift = context->backend.block_size_shift;
	MSG_INFO("Block Size: 0x%.8x (shift: %u)\n",
		 1 << context->backend.block_size_shift, context->backend.block_size_shift);

	io->resp.v2.timeout = get_suggested_timeout(context);

	return lpc_map_memory(context);
}

static int protocol_v2_get_flash_info(struct mbox_context *context,
				      struct protocol_get_flash_info *io)
{
	struct backend *backend = &context->backend;

	io->resp.v2.flash_size =
		backend->flash_size >> backend->block_size_shift;
	io->resp.v2.erase_size =
		((1 << backend->erase_size_shift) >> backend->block_size_shift);

	return 0;
}

static int protocol_v2_create_window(struct mbox_context *context,
				     struct protocol_create_window *io)
{
	int rc;

	rc = protocol_v1_create_window(context, io);
	if (rc < 0)
		return rc;

	io->resp.size = context->current->size >> context->backend.block_size_shift;
	io->resp.offset = context->current->flash_offset >>
					context->backend.block_size_shift;

	return 0;
}

static int protocol_v2_mark_dirty(struct mbox_context *context,
				  struct protocol_mark_dirty *io)
{
	if (!(context->current && context->current_is_write)) {
		MSG_ERR("Tried to call mark dirty without open write window\n");
		return -EPERM;
	}

	MSG_INFO("Dirty window @ 0x%.8x for 0x%.8x\n",
		 io->req.v2.offset << context->backend.block_size_shift,
		 io->req.v2.size << context->backend.block_size_shift);

	return window_set_bytemap(context, context->current, io->req.v2.offset,
				  io->req.v2.size, WINDOW_DIRTY);
}

static int protocol_v2_erase(struct mbox_context *context,
			     struct protocol_erase *io)
{
	size_t start, len;
	int rc;

	if (!(context->current && context->current_is_write)) {
		MSG_ERR("Tried to call erase without open write window\n");
		return -EPERM;
	}

	MSG_INFO("Erase window @ 0x%.8x for 0x%.8x\n",
		 io->req.offset << context->backend.block_size_shift,
		 io->req.size << context->backend.block_size_shift);

	rc = window_set_bytemap(context, context->current, io->req.offset,
				io->req.size, WINDOW_ERASED);
	if (rc < 0) {
		return rc;
	}

	/* Write 0xFF to mem -> This ensures consistency between flash & ram */
	start = io->req.offset << context->backend.block_size_shift;
	len = io->req.size << context->backend.block_size_shift;
	memset(context->current->mem + start, 0xFF, len);

	return 0;
}

static int protocol_v2_flush(struct mbox_context *context,
			     struct protocol_flush *io)
{
	if (!(context->current && context->current_is_write)) {
		MSG_ERR("Tried to call flush without open write window\n");
		return -EPERM;
	}

	return generic_flush(context);
}

static int protocol_v2_close(struct mbox_context *context,
			     struct protocol_close *io)
{
	int rc;

	/* Close the current window if there is one */
	if (!context->current) {
		return 0;
	}

	/* There is an implicit flush if it was a write window */
	if (context->current_is_write) {
		rc = protocol_v2_flush(context, NULL);
		if (rc < 0) {
			MSG_ERR("Couldn't Flush Write Window\n");
			return rc;
		}
	}

	/* Host asked for it -> Don't set the BMC Event */
	windows_close_current(context, io->req.flags);

	return 0;
}

static const struct protocol_ops protocol_ops_v1 = {
	.reset = protocol_v1_reset,
	.get_info = protocol_v1_get_info,
	.get_flash_info = protocol_v1_get_flash_info,
	.create_window = protocol_v1_create_window,
	.mark_dirty = protocol_v1_mark_dirty,
	.erase = NULL,
	.flush = protocol_v1_flush,
	.close = protocol_v1_close,
	.ack = protocol_v1_ack,
};

static const struct protocol_ops protocol_ops_v2 = {
	.reset = protocol_v1_reset,
	.get_info = protocol_v2_get_info,
	.get_flash_info = protocol_v2_get_flash_info,
	.create_window = protocol_v2_create_window,
	.mark_dirty = protocol_v2_mark_dirty,
	.erase = protocol_v2_erase,
	.flush = protocol_v2_flush,
	.close = protocol_v2_close,
	.ack = protocol_v1_ack,
};

static const struct protocol_ops *protocol_ops_map[] = {
	[0] = NULL,
	[1] = &protocol_ops_v1,
	[2] = &protocol_ops_v2,
};

static int protocol_negotiate_version(struct mbox_context *context,
				      uint8_t requested)
{
	/* Check we support the version requested */
	if (requested < API_MIN_VERSION)
		return -EINVAL;

	context->version = (requested > API_MAX_VERSION) ?
				API_MAX_VERSION : requested;

	context->protocol = protocol_ops_map[context->version];

	return context->version;
}

int protocol_init(struct mbox_context *context)
{
	protocol_negotiate_version(context, API_MAX_VERSION);

	return 0;
}

void protocol_free(struct mbox_context *context)
{
	return;
}

/* Don't do any state manipulation, just perform the reset */
int __protocol_reset(struct mbox_context *context)
{
	enum backend_reset_mode mode;
	int rc;

	windows_reset_all(context);

	rc = backend_reset(&context->backend, context->mem, context->mem_size);
	if (rc < 0)
		return rc;

	mode = rc;
	if (!(mode == reset_lpc_flash || mode == reset_lpc_memory))
		return -EINVAL;

	if (mode == reset_lpc_flash)
		return lpc_map_flash(context);

	assert(mode == reset_lpc_memory);
	return lpc_map_memory(context);
}

/* Prevent the host from performing actions whilst reset takes place */
int protocol_reset(struct mbox_context *context)
{
	int rc;

	rc = protocol_events_clear(context, BMC_EVENT_DAEMON_READY);
	if (rc < 0) {
		MSG_ERR("Failed to clear daemon ready state, reset failed\n");
		return rc;
	}

	rc = __protocol_reset(context);
	if (rc < 0) {
		MSG_ERR("Failed to reset protocol, daemon remains not ready\n");
		return rc;
	}

	rc = protocol_events_set(context,
			BMC_EVENT_DAEMON_READY | BMC_EVENT_PROTOCOL_RESET);
	if (rc < 0) {
		MSG_ERR("Failed to set daemon ready state, daemon remains not ready\n");
		return rc;
	}

	return 0;
}
