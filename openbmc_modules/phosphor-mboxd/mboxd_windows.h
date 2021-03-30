/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef MBOXD_WINDOWS_H
#define MBOXD_WINDOWS_H

#define NO_FLUSH	false
#define WITH_FLUSH	true

#include "mbox.h"

/* Initialisation Functions */
int init_windows(struct mbox_context *context);
void free_windows(struct mbox_context *context);
/* Write From Window Functions */
int write_from_window_v1(struct mbox_context *context,
			 uint32_t offset_bytes, uint32_t count_bytes);
int write_from_window(struct mbox_context *context, uint32_t offset,
		      uint32_t count, uint8_t type);
/* Window Management Functions */
void alloc_window_dirty_bytemap(struct mbox_context *context);
int set_window_bytemap(struct mbox_context *context, struct window_context *cur,
		       uint32_t offset, uint32_t size, uint8_t val);
void close_current_window(struct mbox_context *context, bool set_bmc_event,
			  uint8_t flags);
void reset_window(struct mbox_context *context, struct window_context *window);
void reset_all_windows(struct mbox_context *context, bool set_bmc_event);
struct window_context *find_oldest_window(struct mbox_context *context);
struct window_context *find_largest_window(struct mbox_context *context);
struct window_context *search_windows(struct mbox_context *context,
				      uint32_t offset, bool exact);
int create_map_window(struct mbox_context *context,
		      struct window_context **this_window,
		      uint32_t offset, bool exact);

#endif /* MBOXD_WINDOWS_H */
