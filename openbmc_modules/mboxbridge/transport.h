/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef TRANSPORT_H
#define TRANSPORT_H

struct mbox_context;

struct transport_ops {
	int (*put_events)(struct mbox_context *context, uint8_t mask);
	int (*set_events)(struct mbox_context *context, uint8_t events,
			  uint8_t mask);
	int (*clear_events)(struct mbox_context *context, uint8_t events,
			    uint8_t mask);
};

#endif /* TRANSPORT_H */
