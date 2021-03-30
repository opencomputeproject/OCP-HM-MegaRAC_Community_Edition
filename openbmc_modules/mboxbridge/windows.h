/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef WINDOWS_H
#define WINDOWS_H

#include <stdbool.h>

#define WINDOWS_NO_FLUSH	false
#define WINDOWS_WITH_FLUSH	true

struct mbox_context;

/* Window Dirty/Erase bytemap masks */
#define WINDOW_CLEAN			0x00
#define WINDOW_DIRTY			0x01
#define WINDOW_ERASED			0x02

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

/* Initialisation Functions */
int windows_init(struct mbox_context *context);
void windows_free(struct mbox_context *context);
/* Write From Window Functions */
int window_flush_v1(struct mbox_context *context,
			 uint32_t offset_bytes, uint32_t count_bytes);
int window_flush(struct mbox_context *context, uint32_t offset,
		      uint32_t count, uint8_t type);
/* Window Management Functions */
void windows_alloc_dirty_bytemap(struct mbox_context *context);
int window_set_bytemap(struct mbox_context *context, struct window_context *cur,
		       uint32_t offset, uint32_t size, uint8_t val);
void windows_close_current(struct mbox_context *context, uint8_t flags);
void window_reset(struct mbox_context *context, struct window_context *window);
bool windows_reset_all(struct mbox_context *context);
struct window_context *windows_find_oldest(struct mbox_context *context);
struct window_context *windows_find_largest(struct mbox_context *context);
struct window_context *windows_search(struct mbox_context *context,
				      uint32_t offset, bool exact);
int windows_create_map(struct mbox_context *context,
		      struct window_context **this_window,
		      uint32_t offset, bool exact);

#endif /* WINDOWS_H */
