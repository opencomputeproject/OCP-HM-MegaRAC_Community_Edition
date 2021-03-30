/*
 * Mailbox Daemon LPC Helpers
 *
 * Copyright 2017 IBM
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "mbox.h"
#include "mboxd_lpc.h"
#include "mboxd_pnor_partition_table.h"

/*
 * reset_lpc() - Reset the lpc bus mapping
 * @context:     The mbox context pointer
 *
 * Return        0 on success otherwise negative error code
 */
int reset_lpc(struct mbox_context *context)
{
    int rc;

    destroy_vpnor(context);

    rc = init_vpnor(context);
    if (rc < 0)
        return rc;

    rc = vpnor_copy_bootloader_partition(context);
    if (rc < 0)
        return rc;

    return point_to_memory(context);
}
