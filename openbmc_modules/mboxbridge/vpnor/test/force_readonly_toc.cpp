// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include "config.h"

extern "C" {
#include "backend.h"
#include "test/mbox.h"
#include "test/system.h"
#include "vpnor/ffs.h"
}

#include "vpnor/table.hpp"
#include "vpnor/test/tmpd.hpp"

#include <endian.h>
#include <sys/mman.h>

#include <cassert>
#include <cstring>

static constexpr auto BLOCK_SIZE = 4 * 1024;
static constexpr auto PNOR_SIZE = BLOCK_SIZE;
static constexpr auto MEM_SIZE = BLOCK_SIZE;
static constexpr auto ERASE_SIZE = BLOCK_SIZE;
static constexpr auto N_WINDOWS = 1;
static constexpr auto WINDOW_SIZE = BLOCK_SIZE;
static constexpr auto TOC_PART_SIZE = BLOCK_SIZE;

const std::string toc[] = {
    "partition00=part,00000000,00001000,80,READWRITE",
    "partition01=ONE,00001000,00002000,80,READWRITE",
};

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

static const uint8_t read_toc[] = {0x04, 0x04, 0x00, 0x00, 0x01, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

int main()
{
    namespace test = openpower::virtual_pnor::test;
    namespace vpnor = openpower::virtual_pnor;

    struct pnor_partition_table* htable;
    struct mbox_context* ctx;
    uint32_t perms;
    int rc;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(PNOR_SIZE, ERASE_SIZE);

    ctx = mbox_create_frontend_context(N_WINDOWS, WINDOW_SIZE);
    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);
    vpnor::partition::Table table(&ctx->backend);

    assert(table.capacity() == TOC_PART_SIZE);

    rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == MBOX_R_SUCCESS);

    rc = mbox_command_dispatch(ctx, read_toc, sizeof(read_toc));
    assert(rc == MBOX_R_SUCCESS);

    htable = reinterpret_cast<struct pnor_partition_table*>(ctx->mem);
    perms = be32toh(htable->partitions[0].data.user.data[1]);
    assert(perms & PARTITION_READONLY);

    htable = reinterpret_cast<struct pnor_partition_table*>(ctx->mem);
    perms = be32toh(htable->partitions[1].data.user.data[1]);
    assert(!(perms & PARTITION_READONLY));

    return 0;
}
