extern "C" {
#include "mbox.h"

extern const mboxd_mbox_handler vpnor_mbox_handlers[NUM_MBOX_CMDS];

int vpnor_handle_write_window(struct mbox_context *context,
                              union mbox_regs *req, struct mbox_msg *resp);
};
