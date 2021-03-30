/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef LPC_H
#define LPC_H

struct mbox_context;

int lpc_dev_init(struct mbox_context *context);
void lpc_dev_free(struct mbox_context *context);
int lpc_map_flash(struct mbox_context *context);
int lpc_map_memory(struct mbox_context *context);

#endif /* LPC_H */
