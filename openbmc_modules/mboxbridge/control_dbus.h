#ifndef DBUS_CONTROL_H
#define DBUS_CONTROL_H

struct mbox_context;
struct backend;

int control_dbus_init(struct mbox_context *context);
void control_dbus_free(struct mbox_context *context);

int control_legacy_init(struct mbox_context *context);
void control_legacy_free(struct mbox_context *context);

/* Control actions */
int control_ping(struct mbox_context *context);
int control_daemon_state(struct mbox_context *context);
int control_lpc_state(struct mbox_context *context);
int control_reset(struct mbox_context *context);
int control_kill(struct mbox_context *context);
int control_modified(struct mbox_context *context);
int control_suspend(struct mbox_context *context);
int control_resume(struct mbox_context *context, bool modified);
int control_set_backend(struct mbox_context *context, struct backend *backend, void *data);

#endif
