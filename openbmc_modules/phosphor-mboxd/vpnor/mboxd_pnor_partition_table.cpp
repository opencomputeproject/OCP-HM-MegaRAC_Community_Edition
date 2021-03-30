// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include "mboxd_pnor_partition_table.h"
#include "pnor_partition_table.hpp"
#include "common.h"
#include "mbox.h"
#include "mboxd_flash.h"
#include "pnor_partition_table.hpp"
#include "config.h"
#include "xyz/openbmc_project/Common/error.hpp"
#include <phosphor-logging/elog-errors.hpp>
#include <experimental/filesystem>
#include "vpnor/mboxd_msg.hpp"

int init_vpnor(struct mbox_context *context)
{
    if (context && !context->vpnor)
    {
        int rc;

        strncpy(context->paths.ro_loc, PARTITION_FILES_RO_LOC, PATH_MAX);
        context->paths.ro_loc[PATH_MAX - 1] = '\0';
        strncpy(context->paths.rw_loc, PARTITION_FILES_RW_LOC, PATH_MAX);
        context->paths.rw_loc[PATH_MAX - 1] = '\0';
        strncpy(context->paths.prsv_loc, PARTITION_FILES_PRSV_LOC, PATH_MAX);
        context->paths.prsv_loc[PATH_MAX - 1] = '\0';
        strncpy(context->paths.patch_loc, PARTITION_FILES_PATCH_LOC, PATH_MAX);
        context->paths.prsv_loc[PATH_MAX - 1] = '\0';

        rc = init_vpnor_from_paths(context);
        if (rc < 0)
        {
            return rc;
        }
    }

    return 0;
}

int init_vpnor_from_paths(struct mbox_context *context)
{
    namespace err = sdbusplus::xyz::openbmc_project::Common::Error;
    namespace fs = std::experimental::filesystem;
    namespace vpnor = openpower::virtual_pnor;

    if (context && !context->vpnor)
    {
        context->handlers = vpnor_mbox_handlers;

        try
        {
            context->vpnor = new vpnor_partition_table;
            context->vpnor->table =
                new openpower::virtual_pnor::partition::Table(context);
        }
        catch (vpnor::TocEntryError &e)
        {
            MSG_ERR("%s\n", e.what());
            phosphor::logging::commit<err::InternalFailure>();
            return -MBOX_R_SYSTEM_ERROR;
        }
    }

    return 0;
}

int vpnor_copy_bootloader_partition(const struct mbox_context *context)
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
        struct mbox_context local = *context;
        local.vpnor = &vtbl;
        local.block_size_shift = log_2(eraseSize);

        openpower::virtual_pnor::partition::Table blTable(&local);

        vtbl.table = &blTable;

        size_t tocOffset = 0;

        // Copy TOC
        copy_flash(&local, tocOffset,
                   static_cast<uint8_t *>(context->mem) + tocStart,
                   blTable.capacity());
        const pnor_partition &partition = blTable.partition(blPartitionName);
        size_t hbbOffset = partition.data.base * eraseSize;
        uint32_t hbbSize = partition.data.actual;
        // Copy HBB
        copy_flash(&local, hbbOffset,
                   static_cast<uint8_t *>(context->mem) + hbbOffset, hbbSize);
    }
    catch (err::InternalFailure &e)
    {
        phosphor::logging::commit<err::InternalFailure>();
        return -MBOX_R_SYSTEM_ERROR;
    }
    catch (vpnor::ReasonedError &e)
    {
        MSG_ERR("%s\n", e.what());
        phosphor::logging::commit<err::InternalFailure>();
        return -MBOX_R_SYSTEM_ERROR;
    }

    return 0;
}

void destroy_vpnor(struct mbox_context *context)
{
    if (context && context->vpnor)
    {
        delete context->vpnor->table;
        delete context->vpnor;
        context->vpnor = nullptr;
    }
}
