/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef MBOXD_DBUS_H
#define MBOXD_DBUS_H

int init_mboxd_dbus(struct mbox_context *context);
void free_mboxd_dbus(struct mbox_context *context);

#endif /* MBOXD_DBUS_H */
