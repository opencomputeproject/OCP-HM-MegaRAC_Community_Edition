/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef MBOXD_LPC_H
#define MBOXD_LPC_H

#ifdef __cplusplus
extern "C" {
#endif

int init_lpc_dev(struct mbox_context *context);
void free_lpc_dev(struct mbox_context *context);
int point_to_flash(struct mbox_context *context);
int point_to_memory(struct mbox_context *context);
int reset_lpc(struct mbox_context *context);

#ifdef __cplusplus
}
#endif

#endif /* MBOXD_LPC_H */
