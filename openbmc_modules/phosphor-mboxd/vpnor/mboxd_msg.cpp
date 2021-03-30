#include "config.h"

extern "C" {
#include "mbox.h"
#include "mboxd_msg.h"
};

#include "vpnor/mboxd_msg.hpp"
#include "vpnor/pnor_partition_table.hpp"

// clang-format off
const mboxd_mbox_handler vpnor_mbox_handlers[NUM_MBOX_CMDS] =
{
	mbox_handle_reset,
	mbox_handle_mbox_info,
	mbox_handle_flash_info,
	mbox_handle_read_window,
	mbox_handle_close_window,
	vpnor_handle_write_window,
	mbox_handle_dirty_window,
	mbox_handle_flush_window,
	mbox_handle_ack,
	mbox_handle_erase_window
};
// clang-format on

/* XXX: Maybe this should be a method on a class? */
static bool vpnor_partition_is_readonly(const pnor_partition& part)
{
    return part.data.user.data[1] & PARTITION_READONLY;
}

int vpnor_handle_write_window(struct mbox_context* context,
                              union mbox_regs* req, struct mbox_msg* resp)
{
    size_t offset = get_u16(&req->msg.args[0]);
    offset <<= context->block_size_shift;
    try
    {
        const pnor_partition& part = context->vpnor->table->partition(offset);
        if (vpnor_partition_is_readonly(part))
        {
            return -MBOX_R_WINDOW_ERROR;
        }
    }
    catch (const openpower::virtual_pnor::UnmappedOffset& e)
    {
        /*
         * Writes to unmapped areas are not meaningful, so deny the request.
         * This removes the ability for a compromised host to abuse unused
         * space if any data was to be persisted (which it isn't).
         */
        return -MBOX_R_WINDOW_ERROR;
    }

    /* Defer to the default handler */
    return mbox_handle_write_window(context, req, resp);
}
