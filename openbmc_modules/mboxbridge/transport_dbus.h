/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef TRANSPORT_DBUS_H
#define TRANSPORT_DBUS_H

#include "dbus.h"
#include "transport.h"

int transport_dbus_init(struct mbox_context *context,
			const struct transport_ops **ops);
void transport_dbus_free(struct mbox_context *context);

#endif /* TRANSPORT_DBUS_H */
