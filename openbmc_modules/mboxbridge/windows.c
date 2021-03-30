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
#include <mtd/mtd-abi.h>

#include "mboxd.h"
#include "common.h"
#include "transport_mbox.h"
#include "windows.h"
#include "backend.h"

/* Initialisation Functions */

/*
 * init_window_state() - Initialise a new window to a known state
 * @window:	The window to initialise
 * @size:	The size of the window
 */
static void init_window_state(struct window_context *window, uint32_t size)
{
	window->mem = NULL;
	window->flash_offset = FLASH_OFFSET_UNINIT;
	window->size = size;
	window->dirty_bmap = NULL;
	window->age = 0;
}

/*
 * init_window_mem() - Divide the reserved memory region among the windows
 * @context:	The mbox context pointer
 *
 * Return:	0 on success otherwise negative error code
 */
static int init_window_mem(struct mbox_context *context)
{
	void *mem_location = context->mem;
	int i;

	/*
	 * Carve up the reserved memory region and allocate it to each of the
	 * windows. The windows are placed one after the other in ascending
	 * order, so the first window will be first in memory and so on. We
	 * shouldn't have allocated more windows than we have memory, but if we
	 * did we will error out here
	 */
	for (i = 0; i < context->windows.num; i++) {
		uint32_t size = context->windows.window[i].size;
		MSG_DBG("Window %d @ %p for size 0x%.8x\n", i,
			mem_location, size);
		context->windows.window[i].mem = mem_location;
		mem_location += size;
		if (mem_location > (context->mem + context->mem_size)) {
			/* Tried to allocate window past the end of memory */
			MSG_ERR("Total size of windows exceeds reserved mem\n");
			MSG_ERR("Try smaller or fewer windows\n");
			MSG_ERR("Mem size: 0x%.8x\n", context->mem_size);
			return -1;
		}
	}

	return 0;
}
/*
 * windows_init() - Initalise the window cache
 * @context:    The mbox context pointer
 *
 * Return:      0 on success otherwise negative
 */
int windows_init(struct mbox_context *context)
{
	int i;

	/* Check if window size and number set - otherwise set to default */
	if (!context->windows.default_size) {
		/* Default to 1MB windows */
		context->windows.default_size = 1 << 20;
	}
	MSG_INFO("Window size: 0x%.8x\n", context->windows.default_size);
	if (!context->windows.num) {
		/* Use the entire reserved memory region by default */
		context->windows.num = context->mem_size /
				       context->windows.default_size;
	}
	MSG_INFO("Number of windows: %d\n", context->windows.num);

	context->windows.window = calloc(context->windows.num,
					 sizeof(*context->windows.window));
	if (!context->windows.window) {
		MSG_ERR("Memory allocation failed\n");
		return -1;
	}

	for (i = 0; i < context->windows.num; i++) {
		init_window_state(&context->windows.window[i],
				  context->windows.default_size);
	}

	return init_window_mem(context);
}

/*
 * windows_free() - Free the window cache
 * @context:	The mbox context pointer
 */
void windows_free(struct mbox_context *context)
{
	int i;

	/* Check window cache has actually been allocated */
	if (context->windows.window) {
		for (i = 0; i < context->windows.num; i++) {
			free(context->windows.window[i].dirty_bmap);
		}
		free(context->windows.window);
	}
}

/* Write from Window Functions */

/*
 * window_flush_v1() - Handle writing when erase and block size differ
 * @context:		The mbox context pointer
 * @offset_bytes:	The offset in the current window to write from (bytes)
 * @count_bytes:	Number of bytes to write
 *
 * Handle a window_flush for dirty memory when block_size is less than the
 * flash erase size
 * This requires us to be a bit careful because we might have to erase more
 * than we want to write which could result in data loss if we don't have the
 * entire portion of flash to be erased already saved in memory (for us to
 * write back after the erase)
 *
 * Return:	0 on success otherwise negative error code
 */
int window_flush_v1(struct mbox_context *context,
			 uint32_t offset_bytes, uint32_t count_bytes)
{
	int rc;
	uint32_t flash_offset;
	struct window_context low_mem = { 0 }, high_mem = { 0 };

	/* Find where in phys flash this is based on the window.flash_offset */
	flash_offset = context->current->flash_offset + offset_bytes;

	/*
	 * low_mem.flash_offset = erase boundary below where we're writing
	 * low_mem.size = size from low_mem.flash_offset to where we're writing
	 *
	 * high_mem.flash_offset = end of where we're writing
	 * high_mem.size = size from end of where we're writing to next erase
	 * 		   boundary
	 */
	low_mem.flash_offset = align_down(flash_offset,
					  1 << context->backend.erase_size_shift);
	low_mem.size = flash_offset - low_mem.flash_offset;
	high_mem.flash_offset = flash_offset + count_bytes;
	high_mem.size = align_up(high_mem.flash_offset,
				 1 << context->backend.erase_size_shift) -
			high_mem.flash_offset;

	/*
	 * Check if we already have a copy of the required flash areas in
	 * memory as part of the existing window
	 */
	if (low_mem.flash_offset < context->current->flash_offset) {
		/* Before the start of our current window */
		low_mem.mem = malloc(low_mem.size);
		if (!low_mem.mem) {
			MSG_ERR("Unable to allocate memory\n");
			return -ENOMEM;
		}
		rc = backend_copy(&context->backend, low_mem.flash_offset,
				  low_mem.mem, low_mem.size);
		if (rc < 0) {
			goto out;
		}
	}
	if ((high_mem.flash_offset + high_mem.size) >
	    (context->current->flash_offset + context->current->size)) {
		/* After the end of our current window */
		high_mem.mem = malloc(high_mem.size);
		if (!high_mem.mem) {
			MSG_ERR("Unable to allocate memory\n");
			rc = -ENOMEM;
			goto out;
		}
		rc = backend_copy(&context->backend, high_mem.flash_offset,
				  high_mem.mem, high_mem.size);
		if (rc < 0) {
			goto out;
		}
	}

	/*
	 * We need to erase the flash from low_mem.flash_offset->
	 * high_mem.flash_offset + high_mem.size
	 */
	rc = backend_erase(&context->backend, low_mem.flash_offset,
			   (high_mem.flash_offset - low_mem.flash_offset) +
			   high_mem.size);
	if (rc < 0) {
		MSG_ERR("Couldn't erase flash\n");
		goto out;
	}

	/* Write back over the erased area */
	if (low_mem.mem) {
		/* Exceed window at the start */
		rc = backend_write(&context->backend, low_mem.flash_offset,
				   low_mem.mem, low_mem.size);
		if (rc < 0) {
			goto out;
		}
	}
	rc = backend_write(&context->backend, flash_offset,
			 context->current->mem + offset_bytes, count_bytes);
	if (rc < 0) {
		goto out;
	}
	/*
	 * We still need to write the last little bit that we erased - it's
	 * either in the current window or the high_mem window.
	 */
	if (high_mem.mem) {
		/* Exceed window at the end */
		rc = backend_write(&context->backend, high_mem.flash_offset,
				   high_mem.mem, high_mem.size);
		if (rc < 0) {
			goto out;
		}
	} else {
		/* Write from the current window - it's atleast that big */
		rc = backend_write(&context->backend, high_mem.flash_offset,
				   context->current->mem + offset_bytes +
				   count_bytes, high_mem.size);
		if (rc < 0) {
			goto out;
		}
	}

out:
	free(low_mem.mem);
	free(high_mem.mem);
	return rc;
}

/*
 * window_flush() - Write back to the flash from the current window
 * @context:		The mbox context pointer
 * @offset_bytes:	The offset in the current window to write from (blocks)
 * @count_bytes:	Number of blocks to write
 * @type:		Whether this is an erase & write or just an erase
 *
 * Return:	0 on success otherwise negative error code
 */
int window_flush(struct mbox_context *context, uint32_t offset,
		      uint32_t count, uint8_t type)
{
	int rc;
	uint32_t flash_offset, count_bytes = count << context->backend.block_size_shift;
	uint32_t offset_bytes = offset << context->backend.block_size_shift;

	switch (type) {
	case WINDOW_ERASED: /* >= V2 ONLY -> block_size == erasesize */
		flash_offset = context->current->flash_offset + offset_bytes;
		rc = backend_erase(&context->backend, flash_offset,
				   count_bytes);
		if (rc < 0) {
			MSG_ERR("Couldn't erase flash\n");
			return rc;
		}
		break;
	case WINDOW_DIRTY:
		/*
		 * For protocol V1, block_size may be smaller than erase size
		 * so we have a special function to make sure that we do this
		 * correctly without losing data.
		 */
		if (context->backend.erase_size_shift !=
				context->backend.block_size_shift) {
			return window_flush_v1(context, offset_bytes,
						    count_bytes);
		}
		flash_offset = context->current->flash_offset + offset_bytes;

		/* Erase the flash */
		rc = backend_erase(&context->backend, flash_offset,
				   count_bytes);
		if (rc < 0) {
			return rc;
		}

		/* Write to the erased flash */
		rc = backend_write(&context->backend, flash_offset,
				   context->current->mem + offset_bytes,
				   count_bytes);
		if (rc < 0) {
			return rc;
		}

		break;
	default:
		/* We shouldn't be able to get here */
		MSG_ERR("Write from window with invalid type: %d\n", type);
		return -EPERM;
	}

	return 0;
}

/* Window Management Functions */

/*
 * windows_alloc_dirty_bytemap() - (re)allocate all the window dirty bytemaps
 * @context:		The mbox context pointer
 */
void windows_alloc_dirty_bytemap(struct mbox_context *context)
{
	struct window_context *cur;
	int i;

	for (i = 0; i < context->windows.num; i++) {
		cur = &context->windows.window[i];
		/* There may already be one allocated */
		free(cur->dirty_bmap);
		/* Allocate the new one */
		cur->dirty_bmap = calloc((context->windows.default_size >>
					  context->backend.block_size_shift),
					 sizeof(*cur->dirty_bmap));
	}
}

/*
 * window_set_bytemap() - Set the window bytemap
 * @context:	The mbox context pointer
 * @cur:	The window to set the bytemap of
 * @offset:	Where in the window to set the bytemap (blocks)
 * @size:	The number of blocks to set
 * @val:	The value to set the bytemap to
 *
 * Return:	0 on success otherwise negative error code
 */
int window_set_bytemap(struct mbox_context *context, struct window_context *cur,
		       uint32_t offset, uint32_t size, uint8_t val)
{
	if (offset + size > (cur->size >> context->backend.block_size_shift)) {
		MSG_ERR("Tried to set window bytemap past end of window\n");
		MSG_ERR("Requested offset: 0x%x size: 0x%x window size: 0x%x\n",
			offset << context->backend.block_size_shift,
			size << context->backend.block_size_shift,
			cur->size << context->backend.block_size_shift);
		return -EACCES;
	}

	memset(cur->dirty_bmap + offset, val, size);
	return 0;
}

/*
 * windows_close_current() - Close the current (active) window
 * @context:   		The mbox context pointer
 * @flags:		Flags as defined for a close command in the protocol
 *
 * This closes the current window. If the host has requested the current window
 * be closed then we don't need to set the bmc event bit
 * (set_bmc_event == false), otherwise if the current window has been closed
 * without the host requesting it the bmc event bit must be set to indicate this
 * to the host (set_bmc_event == true).
 */
void windows_close_current(struct mbox_context *context, uint8_t flags)
{
	MSG_DBG("Close current window, flags: 0x%.2x\n", flags);

	if (flags & FLAGS_SHORT_LIFETIME) {
		context->current->age = 0;
	}

	context->current = NULL;
	context->current_is_write = false;
}

/*
 * window_reset() - Reset a window context to a well defined default state
 * @context:   	The mbox context pointer
 * @window:	The window to reset
 */
void window_reset(struct mbox_context *context, struct window_context *window)
{
	window->flash_offset = FLASH_OFFSET_UNINIT;
	window->size = context->windows.default_size;
	if (window->dirty_bmap) { /* Might not have been allocated */
		window_set_bytemap(context, window, 0,
				   window->size >> context->backend.block_size_shift,
				   WINDOW_CLEAN);
	}
	window->age = 0;
}

/*
 * windows_reset_all() - Reset all windows to a well defined default state
 * @context:		The mbox context pointer
 *
 * @return True if there was a window open that was closed, false otherwise
 */
bool windows_reset_all(struct mbox_context *context)
{
	bool closed = context->current;
	int i;

	MSG_DBG("Resetting all windows\n");

	context->windows.max_age = 0;

	/* We might have an open window which needs closing */

	if (context->current) {
		windows_close_current(context, FLAGS_NONE);
	}

	for (i = 0; i < context->windows.num; i++) {
		window_reset(context, &context->windows.window[i]);
	}

	return closed;
}

/*
 * windows_find_oldest() - Find the oldest (Least Recently Used) window
 * @context:		The mbox context pointer
 *
 * Return:	Pointer to the least recently used window
 */
struct window_context *windows_find_oldest(struct mbox_context *context)
{
	struct window_context *oldest = NULL, *cur;
	uint32_t min_age = context->windows.max_age + 1;
	int i;

	for (i = 0; i < context->windows.num; i++) {
		cur = &context->windows.window[i];

		if (cur->age < min_age) {
			min_age = cur->age;
			oldest = cur;
		}
	}

	return oldest;
}

/*
 * windows_find_largest() - Find the largest window in the window cache
 * @context:	The mbox context pointer
 *
 * Return:	The largest window
 */
struct window_context *windows_find_largest(struct mbox_context *context)
{
	struct window_context *largest = NULL, *cur;
	uint32_t max_size = 0;
	int i;

	for (i = 0; i < context->windows.num; i++) {
		cur = &context->windows.window[i];

		if (cur->size > max_size) {
			max_size = cur->size;
			largest = cur;
		}
	}

	return largest;
}

/*
 * windows_search() - Search the window cache for a window containing offset
 * @context:	The mbox context pointer
 * @offset:	Absolute flash offset to search for (bytes)
 * @exact:	If the window must exactly map the requested offset
 *
 * This will search the cache of windows for one containing the requested
 * offset. For V1 of the protocol windows must exactly map the offset since we
 * can't tell the host how much of its request we actually mapped and it will
 * thus assume it can access window->size from the offset we give it.
 *
 * Return:	Pointer to a window containing the requested offset otherwise
 *		NULL
 */
struct window_context *windows_search(struct mbox_context *context,
				      uint32_t offset, bool exact)
{
	struct window_context *cur;
	int i;

	MSG_DBG("Searching for window which contains 0x%.8x %s\n",
		offset, exact ? "exactly" : "");
	for (i = 0; i < context->windows.num; i++) {
		cur = &context->windows.window[i];
		if (cur->flash_offset == FLASH_OFFSET_UNINIT) {
			/* Uninitialised Window */
			if (offset == FLASH_OFFSET_UNINIT) {
				return cur;
			}
			continue;
		}
		if ((offset >= cur->flash_offset) &&
		    (offset < (cur->flash_offset + cur->size))) {
			if (exact && (cur->flash_offset != offset)) {
				continue;
			}
			/* This window contains the requested offset */
			cur->age = ++(context->windows.max_age);
			return cur;
		}
	}

	return NULL;
}

/*
 * windows_create_map() - Create a window mapping which maps the requested offset
 * @context:		The mbox context pointer
 * @this_window:	A pointer to update to the "new" window
 * @offset:		Absolute flash offset to create a mapping for (bytes)
 * @exact:		If the window must exactly map the requested offset
 *
 * This is used to create a window mapping for the requested offset when there
 * is no existing window in the cache which satisfies the offset. This involves
 * choosing an existing window from the window cache to evict so we can use it
 * to store the flash contents from the requested offset, we then point the
 * caller to that window since it now maps their request.
 *
 * Return:	0 on success otherwise negative error code
 */
int windows_create_map(struct mbox_context *context,
		      struct window_context **this_window, uint32_t offset,
		      bool exact)
{
	struct window_context *cur = NULL;
	int rc;

	MSG_DBG("Creating window which maps 0x%.8x %s\n", offset,
		exact ? "exactly" : "");

	/* Search for an uninitialised window, use this before evicting */
	cur = windows_search(context, FLASH_OFFSET_UNINIT, true);

	/* No uninitialised window found, we need to choose one to "evict" */
	if (!cur) {
		MSG_DBG("No uninitialised window, evicting one\n");
		cur = windows_find_oldest(context);
		window_reset(context, cur);
	}

	/* Adjust the offset for alignment by the backend. It will help prevent the
	 * overlap.
	 */
	if (!exact) {
		if (backend_align_offset(&(context->backend), &offset, cur->size)) {
			MSG_ERR("Can't adjust the offset by backend\n");
		}
	}

	if (offset > context->backend.flash_size) {
		MSG_ERR("Tried to open read window past flash limit\n");
		return -EINVAL;
	} else if ((offset + cur->size) > context->backend.flash_size) {
		/*
		 * There is V1 skiboot implementations out there which don't
		 * mask offset with window size, meaning when we have
		 * window size == flash size we will never allow the host to
		 * open a window except at 0x0, which isn't always where the
		 * host requests it. Thus we have to ignore this check and just
		 * hope the host doesn't access past the end of the window
		 * (which it shouldn't) for V1 implementations to get around
		 * this.
		 */
		if (context->version == API_VERSION_1) {
			cur->size = align_down(context->backend.flash_size - offset,
					       1 << context->backend.block_size_shift);
		} else {
			/*
			 * Allow requests to exceed the flash size, but limit
			 * the response to the size of the flash.
			 */
			cur->size = context->backend.flash_size - offset;
		}
	}

	/* Copy from flash into the window buffer */
	rc = backend_copy(&context->backend, offset, cur->mem, cur->size);
	if (rc < 0) {
		/* We don't know how much we've copied -> better reset window */
		window_reset(context, cur);
		return rc;
	}
	/*
	 * rc isn't guaranteed to be aligned, so align up
	 *
	 * FIXME: This should only be the case for the vpnor ToC now, so handle
	 * it there
	 */
	cur->size = align_up(rc, (1ULL << context->backend.block_size_shift));
	/* Would like a known value, pick 0xFF to it looks like erased flash */
	memset(cur->mem + rc, 0xFF, cur->size - rc);

	/*
	 * Since for V1 windows aren't constrained to start at multiples of
	 * window size it's possible that something already maps this offset.
	 * Reset any windows which map this offset to avoid coherency problems.
	 * We just have to check for anything which maps the start or the end
	 * of the window since all windows are the same size so another window
	 * cannot map just the middle of this window.
	 */
	if (context->version == API_VERSION_1) {
		uint32_t i;

		MSG_DBG("Checking for window overlap\n");

		for (i = offset; i < (offset + cur->size); i += (cur->size - 1)) {
			struct window_context *tmp = NULL;
			do {
				tmp = windows_search(context, i, false);
				if (tmp) {
					window_reset(context, tmp);
				}
			} while (tmp);
		}
	}

	/* Clear the bytemap of the window just loaded -> we know it's clean */
	window_set_bytemap(context, cur, 0,
			   cur->size >> context->backend.block_size_shift,
			   WINDOW_CLEAN);

	/* Update so we know what's in the window */
	cur->flash_offset = offset;
	cur->age = ++(context->windows.max_age);
	*this_window = cur;

	return 0;
}
