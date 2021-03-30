// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "config.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <syslog.h>
#include <unistd.h>

#include <algorithm>

extern "C" {
#include "backend.h"
#include "common.h"
#include "lpc.h"
#include "mboxd.h"
#include "protocol.h"
#include "vpnor/backend.h"
}

#include "vpnor/partition.hpp"
#include "vpnor/table.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <cassert>
#include <exception>
#include <experimental/filesystem>
#include <memory>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <stdexcept>
#include <string>

#include "vpnor/backend.h"

namespace err = sdbusplus::xyz::openbmc_project::Common::Error;
namespace fs = std::experimental::filesystem;
namespace vpnor = openpower::virtual_pnor;

static constexpr uint32_t VPNOR_ERASE_SIZE = 4 * 1024;

void vpnor_default_paths(vpnor_partition_paths* paths)
{
    strncpy(paths->ro_loc, PARTITION_FILES_RO_LOC, PATH_MAX);
    paths->ro_loc[PATH_MAX - 1] = '\0';
    strncpy(paths->rw_loc, PARTITION_FILES_RW_LOC, PATH_MAX);
    paths->rw_loc[PATH_MAX - 1] = '\0';
    strncpy(paths->prsv_loc, PARTITION_FILES_PRSV_LOC, PATH_MAX);
    paths->prsv_loc[PATH_MAX - 1] = '\0';
    strncpy(paths->patch_loc, PARTITION_FILES_PATCH_LOC, PATH_MAX);
    paths->prsv_loc[PATH_MAX - 1] = '\0';
}

/** @brief Create a virtual PNOR partition table.
 *
 *  @param[in] backend - The backend context pointer
 *  @param[in] paths - A paths object pointer to initialise vpnor
 *
 *  This API should be called before calling any other APIs below. If a table
 *  already exists, this function will not do anything further. This function
 *  will not do anything if the context is NULL.
 *
 *  The content of the paths object is copied out, ownership is retained by the
 *  caller.
 *
 *  Returns 0 if the call succeeds, else a negative error code.
 */
static int vpnor_init(struct backend* backend,
                      const vpnor_partition_paths* paths)
{
    namespace err = sdbusplus::xyz::openbmc_project::Common::Error;
    namespace fs = std::experimental::filesystem;
    namespace vpnor = openpower::virtual_pnor;

    vpnor_data* priv = new vpnor_data;
    assert(priv);

    priv->paths = *paths;
    backend->priv = priv;

    try
    {
        priv->vpnor = new vpnor_partition_table;
        priv->vpnor->table =
            new openpower::virtual_pnor::partition::Table(backend);
    }
    catch (vpnor::TocEntryError& e)
    {
        MSG_ERR("%s\n", e.what());
        try
        {
            phosphor::logging::commit<err::InternalFailure>();
        }
        catch (const std::exception& e)
        {
            MSG_ERR("Failed to commit InternalFailure: %s\n", e.what());
        }
        return -EINVAL;
    }

    return 0;
}

/** @brief Copy bootloader partition (alongwith TOC) to LPC memory
 *
 *  @param[in] backend - The backend context pointer
 *
 *  @returns 0 on success, negative error code on failure
 */
int vpnor_copy_bootloader_partition(const struct backend* backend, void* buf,
                                    uint32_t count)
{
    // The hostboot bootloader has certain size/offset assumptions, so
    // we need a special partition table here.
    // It assumes the PNOR is 64M, the TOC size is 32K, the erase block is
    // 4K, the page size is 4K.
    // It also assumes the TOC is at the 'end of pnor - toc size - 1 page size'
    // offset, and first looks for the TOC here, before proceeding to move up
    // page by page looking for the TOC. So it is optimal to place the TOC at
    // this offset.
    constexpr size_t eraseSize = 0x1000;
    constexpr size_t pageSize = 0x1000;
    constexpr size_t pnorSize = 0x4000000;
    constexpr size_t tocMaxSize = 0x8000;
    constexpr size_t tocStart = pnorSize - tocMaxSize - pageSize;
    constexpr auto blPartitionName = "HBB";

    namespace err = sdbusplus::xyz::openbmc_project::Common::Error;
    namespace fs = std::experimental::filesystem;
    namespace vpnor = openpower::virtual_pnor;

    try
    {
        vpnor_partition_table vtbl{};
        struct vpnor_data priv;
        struct backend local = *backend;

        priv.vpnor = &vtbl;
        priv.paths = ((struct vpnor_data*)backend->priv)->paths;
        local.priv = &priv;
        local.block_size_shift = log_2(eraseSize);

        openpower::virtual_pnor::partition::Table blTable(&local);

        vtbl.table = &blTable;

        size_t tocOffset = 0;

        const pnor_partition& partition = blTable.partition(blPartitionName);
        size_t hbbOffset = partition.data.base * eraseSize;
        uint32_t hbbSize = partition.data.actual;

        if (count < tocStart + blTable.capacity() ||
            count < hbbOffset + hbbSize)
        {
            MSG_ERR("Reserved memory too small for dumb bootstrap\n");
            return -EINVAL;
        }

        uint8_t* buf8 = static_cast<uint8_t*>(buf);
        backend_copy(&local, tocOffset, buf8 + tocStart, blTable.capacity());
        backend_copy(&local, hbbOffset, buf8 + hbbOffset, hbbSize);
    }
    catch (err::InternalFailure& e)
    {
        phosphor::logging::commit<err::InternalFailure>();
        return -EIO;
    }
    catch (vpnor::ReasonedError& e)
    {
        MSG_ERR("%s\n", e.what());
        phosphor::logging::commit<err::InternalFailure>();
        return -EIO;
    }

    return 0;
}

int vpnor_dev_init(struct backend* backend, void* data)
{
    vpnor_partition_paths* paths = (vpnor_partition_paths*)data;
    struct mtd_info_user mtd_info;
    const char* filename = NULL;
    int fd;
    int rc = 0;

    if (!(fs::is_directory(fs::status(paths->ro_loc)) &&
          fs::is_directory(fs::status(paths->rw_loc)) &&
          fs::is_directory(fs::status(paths->prsv_loc))))
    {
        return -EINVAL;
    }

    if (backend->flash_size == 0)
    {
        filename = get_dev_mtd();

        MSG_INFO("No flash size provided, using PNOR MTD size\n");

        if (!filename)
        {
            MSG_ERR("Couldn't find the flash /dev/mtd partition\n");
            return -errno;
        }

        MSG_DBG("Opening %s\n", filename);

        fd = open(filename, O_RDWR);
        if (fd < 0)
        {
            MSG_ERR("Couldn't open %s with flags O_RDWR: %s\n", filename,
                    strerror(errno));
            rc = -errno;
            goto cleanup_filename;
        }

        // Read the Flash Info
        if (ioctl(fd, MEMGETINFO, &mtd_info) == -1)
        {
            MSG_ERR("Couldn't get information about MTD: %s\n",
                    strerror(errno));
            rc = -errno;
            goto cleanup_fd;
        }

        close(fd);
        free((void*)filename);

        // See comment in flash.c on why
        // this is needed.
        backend->flash_size = mtd_info.size;
    }

    // Hostboot requires a 4K block-size to be used in the FFS flash structure
    backend->erase_size_shift = log_2(VPNOR_ERASE_SIZE);
    backend->block_size_shift = backend->erase_size_shift;

    return vpnor_init(backend, paths);

cleanup_fd:
    close(fd);

cleanup_filename:
    free((void*)filename);

    return rc;
}

static void vpnor_free(struct backend* backend)
{
    struct vpnor_data* priv = (struct vpnor_data*)backend->priv;

    if (priv)
    {
        if (priv->vpnor)
        {
            delete priv->vpnor->table;
        }
        delete priv->vpnor;
    }
    delete priv;
}

/*
 * vpnor_copy() - Copy data from the virtual pnor into a provided buffer
 * @context:    The backend context pointer
 * @offset:     The pnor offset to copy from (bytes)
 * @mem:        The buffer to copy into (must be of atleast 'size' bytes)
 * @size:       The number of bytes to copy
 * Return:      Number of bytes copied on success, otherwise negative error
 *              code. vpnor_copy will copy at most 'size' bytes, but it may
 *              copy less.
 */
static int64_t vpnor_copy(struct backend* backend, uint32_t offset, void* mem,
                          uint32_t size)
{
    struct vpnor_data* priv = (struct vpnor_data*)backend->priv;
    vpnor::partition::Table* table;
    int rc = size;

    if (!(priv->vpnor && priv->vpnor->table))
    {
        MSG_ERR("Trying to copy data with uninitialised context!\n");
        return -EINVAL;
    }

    table = priv->vpnor->table;

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
        vpnor::Request req(backend, offset);
        rc = req.read(mem, size);
    }
    catch (vpnor::UnmappedOffset& e)
    {
        /*
         * Hooo boy. Pretend that this is valid flash so we don't have
         * discontiguous regions presented to the host. Instead, fill a window
         * with 0xff so the 'flash' looks erased. Writes to such regions are
         * dropped on the floor, see the implementation of vpnor_write() below.
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
        rc = -EIO;
    }
    return rc;
}

/*
 * vpnor_write() - Write to the virtual pnor from a provided buffer
 * @context: The backend context pointer
 * @offset:  The flash offset to write to (bytes)
 * @buf:     The buffer to write from (must be of atleast size)
 * @size:    The number of bytes to write
 *
 * Return:  0 on success otherwise negative error code
 */

static int vpnor_write(struct backend* backend, uint32_t offset, void* buf,
                       uint32_t count)
{
    assert(backend);

    struct vpnor_data* priv = (struct vpnor_data*)backend->priv;

    if (!(priv && priv->vpnor && priv->vpnor->table))
    {
        MSG_ERR("Trying to write data with uninitialised context!\n");
        return -EINVAL;
    }

    vpnor::partition::Table* table = priv->vpnor->table;

    try
    {
        const struct pnor_partition& part = table->partition(offset);
        if (part.data.user.data[1] & PARTITION_READONLY)
        {
            MSG_ERR("Unreachable: Host attempted to write to read-only "
                    "partition %s\n",
                    part.data.name);
            return -EPERM;
        }

        MSG_DBG("Write flash @ 0x%.8x for 0x%.8x from %p\n", offset, count,
                buf);
        vpnor::Request req(backend, offset);
        req.write(buf, count);
    }
    catch (vpnor::UnmappedOffset& e)
    {
        MSG_ERR("Unreachable: Host attempted to write %" PRIu32
                " bytes to unmapped offset 0x%" PRIx32 "\n",
                count, offset);
        return -EACCES;
    }
    catch (const vpnor::OutOfBoundsOffset& e)
    {
        MSG_ERR("%s\n", e.what());
        return -EINVAL;
    }
    catch (const std::exception& e)
    {
        MSG_ERR("%s\n", e.what());
        phosphor::logging::commit<err::InternalFailure>();
        return -EIO;
    }
    return 0;
}

static bool vpnor_partition_is_readonly(const pnor_partition& part)
{
    return part.data.user.data[1] & PARTITION_READONLY;
}

static int vpnor_validate(struct backend* backend, uint32_t offset,
                          uint32_t size __attribute__((unused)), bool ro)
{
    struct vpnor_data* priv = (struct vpnor_data*)backend->priv;

    /* All reads are allowed */
    if (ro)
    {
        return 0;
    }

    /* Only allow write windows on regions mapped by the ToC as writeable */
    try
    {
        const pnor_partition& part = priv->vpnor->table->partition(offset);
        if (vpnor_partition_is_readonly(part))
        {
            MSG_DBG("Try to write read only partition (part=%s, offset=0x%x)\n",
                    part.data.name, offset);
            return -EPERM;
        }
    }
    catch (const openpower::virtual_pnor::UnmappedOffset& e)
    {
        /*
         * Writes to unmapped areas are not meaningful, so deny the request.
         * This removes the ability for a compromised host to abuse unused
         * space if any data was to be persisted (which it isn't).
         */
        return -EACCES;
    }

    // Allowed.
    return 0;
}

/*
 * vpnor_reset() - Reset the lpc bus mapping
 * @context:     The mbox context pointer
 *
 * Return        0 on success otherwise negative error code
 */
static int vpnor_reset(struct backend* backend, void* buf, uint32_t count)
{
    const struct vpnor_data* priv = (const struct vpnor_data*)backend->priv;
    int rc;

    vpnor_partition_paths paths = priv->paths;

    vpnor_free(backend);

    rc = vpnor_init(backend, &paths);
    if (rc < 0)
        return rc;

    rc = vpnor_copy_bootloader_partition(backend, buf, count);
    if (rc < 0)
        return rc;

    return reset_lpc_memory;
}

/*
 * vpnor_align_offset() - Align the offset
 * @context:    The backend context pointer
 * @offset:	The flash offset
 * @window_size:The window size
 *
 * Return:      0 on success otherwise negative error code
 */
static int vpnor_align_offset(struct backend* backend, uint32_t* offset,
                              uint32_t window_size)
{
    const struct vpnor_data* priv = (const struct vpnor_data*)backend->priv;

    /* Adjust the offset to align with the offset of partition base */
    try
    {
        // Get the base of the partition
        const pnor_partition& part = priv->vpnor->table->partition(*offset);
        uint32_t base = part.data.base * VPNOR_ERASE_SIZE;

        // Get the base offset relative to the window_size
        uint32_t base_offset = base & (window_size - 1);

        // Adjust the offset to align with the base
        *offset = ((*offset - base_offset) & ~(window_size - 1)) + base_offset;
        MSG_DBG(
            "vpnor_align_offset: to @ 0x%.8x(base=0x%.8x base_offset=0x%.8x)\n",
            *offset, base, base_offset);
        return 0;
    }
    catch (const openpower::virtual_pnor::UnmappedOffset& e)
    {
        /*
         * Writes to unmapped areas are not meaningful, so deny the request.
         * This removes the ability for a compromised host to abuse unused
         * space if any data was to be persisted (which it isn't).
         */
        return -EACCES;
    }
}

static const struct backend_ops vpnor_ops = {
    .init = vpnor_dev_init,
    .free = vpnor_free,
    .copy = vpnor_copy,
    .set_bytemap = NULL,
    .erase = NULL,
    .write = vpnor_write,
    .validate = vpnor_validate,
    .reset = vpnor_reset,
    .align_offset = vpnor_align_offset,
};

struct backend backend_get_vpnor(void)
{
    struct backend be = {0};

    be.ops = &vpnor_ops;

    return be;
}

int backend_probe_vpnor(struct backend* master,
                        const struct vpnor_partition_paths* paths)
{
    struct backend with;

    assert(master);
    with = backend_get_vpnor();

    return backend_init(master, &with, (void*)paths);
}
