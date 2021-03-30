// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <algorithm>

extern "C" {
#include "common.h"
}

#include "config.h"
#include "mboxd_flash.h"
#include "mboxd_pnor_partition_table.h"
#include "pnor_partition.hpp"
#include "pnor_partition_table.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog-errors.hpp>

#include <memory>
#include <string>
#include <exception>
#include <stdexcept>

namespace err = sdbusplus::xyz::openbmc_project::Common::Error;
namespace fs = std::experimental::filesystem;
namespace vpnor = openpower::virtual_pnor;

/** @brief unique_ptr functor to release a char* reference. */
struct StringDeleter
{
    void operator()(char* ptr) const
    {
        free(ptr);
    }
};
using StringPtr = std::unique_ptr<char, StringDeleter>;

int init_flash_dev(struct mbox_context* context)
{
    StringPtr filename(get_dev_mtd());
    int fd = 0;
    int rc = 0;

    if (!filename)
    {
        MSG_ERR("Couldn't find the flash /dev/mtd partition\n");
        return -1;
    }

    MSG_DBG("Opening %s\n", filename.get());

    fd = open(filename.get(), O_RDWR);
    if (fd < 0)
    {
        MSG_ERR("Couldn't open %s with flags O_RDWR: %s\n", filename.get(),
                strerror(errno));
        return -errno;
    }

    // Read the Flash Info
    if (ioctl(fd, MEMGETINFO, &context->mtd_info) == -1)
    {
        MSG_ERR("Couldn't get information about MTD: %s\n", strerror(errno));
        close(fd);
        return -errno;
    }

    if (context->flash_size == 0)
    {
        // See comment in mboxd_flash_physical.c on why
        // this is needed.
        context->flash_size = context->mtd_info.size;
    }

    // Hostboot requires a 4K block-size to be used in the FFS flash structure
    context->mtd_info.erasesize = 4096;
    context->erase_size_shift = log_2(context->mtd_info.erasesize);
    context->flash_bmap = NULL;
    context->fds[MTD_FD].fd = -1;

    close(fd);
    return rc;
}

void free_flash_dev(struct mbox_context* context)
{
    // No-op
}

int set_flash_bytemap(struct mbox_context* context, uint32_t offset,
                      uint32_t count, uint8_t val)
{
    // No-op
    return 0;
}

int erase_flash(struct mbox_context* context, uint32_t offset, uint32_t count)
{
    // No-op
    return 0;
}

/*
 * copy_flash() - Copy data from the virtual pnor into a provided buffer
 * @context:    The mbox context pointer
 * @offset:     The pnor offset to copy from (bytes)
 * @mem:        The buffer to copy into (must be of atleast 'size' bytes)
 * @size:       The number of bytes to copy
 * Return:      Number of bytes copied on success, otherwise negative error
 *              code. copy_flash will copy at most 'size' bytes, but it may
 *              copy less.
 */
int64_t copy_flash(struct mbox_context* context, uint32_t offset, void* mem,
                   uint32_t size)
{
    vpnor::partition::Table* table;
    int rc = size;

    if (!(context && context->vpnor && context->vpnor->table))
    {
        MSG_ERR("Trying to copy data with uninitialised context!\n");
        return -MBOX_R_SYSTEM_ERROR;
    }

    table = context->vpnor->table;

    MSG_DBG("Copy virtual pnor to %p for size 0x%.8x from offset 0x%.8x\n", mem,
            size, offset);

    /* The virtual PNOR partition table starts at offset 0 in the virtual
     * pnor image. Check if host asked for an offset that lies within the
     * partition table.
     */
    size_t sz = table->size();
    if (offset < sz)
    {
        const pnor_partition_table& toc = table->getHostTable();
        rc = std::min(sz - offset, static_cast<size_t>(size));
        memcpy(mem, ((uint8_t*)&toc) + offset, rc);
        return rc;
    }

    try
    {
        vpnor::Request req(context, offset);
        rc = req.read(mem, size);
    }
    catch (vpnor::UnmappedOffset& e)
    {
        /*
         * Hooo boy. Pretend that this is valid flash so we don't have
         * discontiguous regions presented to the host. Instead, fill a window
         * with 0xff so the 'flash' looks erased. Writes to such regions are
         * dropped on the floor, see the implementation of write_flash() below.
         */
        MSG_INFO("Host requested unmapped region of %" PRId32
                 " bytes at offset 0x%" PRIx32 "\n",
                 size, offset);
        uint32_t span = e.next - e.base;
        rc = std::min(size, span);
        memset(mem, 0xff, rc);
    }
    catch (std::exception& e)
    {
        MSG_ERR("%s\n", e.what());
        phosphor::logging::commit<err::InternalFailure>();
        rc = -MBOX_R_SYSTEM_ERROR;
    }
    return rc;
}

/*
 * write_flash() - Write to the virtual pnor from a provided buffer
 * @context: The mbox context pointer
 * @offset:  The flash offset to write to (bytes)
 * @buf:     The buffer to write from (must be of atleast size)
 * @size:    The number of bytes to write
 *
 * Return:  0 on success otherwise negative error code
 */

int write_flash(struct mbox_context* context, uint32_t offset, void* buf,
                uint32_t count)
{

    if (!(context && context->vpnor && context->vpnor->table))
    {
        MSG_ERR("Trying to write data with uninitialised context!\n");
        return -MBOX_R_SYSTEM_ERROR;
    }

    vpnor::partition::Table* table = context->vpnor->table;

    try
    {
        const struct pnor_partition& part = table->partition(offset);
        if (part.data.user.data[1] & PARTITION_READONLY)
        {
            MSG_ERR("Unreachable: Host attempted to write to read-only "
                    "partition %s\n",
                    part.data.name);
            return -MBOX_R_WRITE_ERROR;
        }

        MSG_DBG("Write flash @ 0x%.8x for 0x%.8x from %p\n", offset, count,
                buf);
        vpnor::Request req(context, offset);
        req.write(buf, count);
    }
    catch (vpnor::UnmappedOffset& e)
    {
        MSG_ERR("Unreachable: Host attempted to write %" PRIu32
                " bytes to unmapped offset 0x%" PRIx32 "\n",
                count, offset);
        return -MBOX_R_WRITE_ERROR;
    }
    catch (const vpnor::OutOfBoundsOffset& e)
    {
        MSG_ERR("%s\n", e.what());
        return -MBOX_R_PARAM_ERROR;
    }
    catch (const std::exception& e)
    {
        MSG_ERR("%s\n", e.what());
        phosphor::logging::commit<err::InternalFailure>();
        return -MBOX_R_SYSTEM_ERROR;
    }
    return 0;
}
