// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include "config.h"

extern "C" {
#include "backend.h"
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/table.hpp"
#include "vpnor/test/tmpd.hpp"

#include <sys/mman.h>

#include <cassert>
#include <cstring>

static constexpr auto BLOCK_SIZE = 4 * 1024;
static constexpr auto PNOR_SIZE = 64 * 1024 * 1024;
static constexpr auto MEM_SIZE = BLOCK_SIZE * 2;
static constexpr auto ERASE_SIZE = BLOCK_SIZE;
static constexpr auto N_WINDOWS = 1;
static constexpr auto WINDOW_SIZE = BLOCK_SIZE;
static constexpr auto TOC_PART_SIZE = BLOCK_SIZE;

const std::string toc[] = {
    "partition00=part,00000000,00001000,80,READONLY",
    "partition01=ONE,00001000,00002000,80,READWRITE",
};

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

/* Request access to the ToC base for one block */
static const uint8_t create_read_window[] = {0x04, 0x01, 0x00, 0x00, 0x01, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00};

/* Expect a response containing the ToC in one block */
static const uint8_t response[] = {
    0x04, 0x01, 0xfe, 0xff, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
};

int main()
{
    namespace test = openpower::virtual_pnor::test;
    namespace vpnor = openpower::virtual_pnor;

    struct mbox_context* ctx;
    int rc;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(PNOR_SIZE, ERASE_SIZE);

    ctx = mbox_create_frontend_context(N_WINDOWS, WINDOW_SIZE);
    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);
    vpnor::partition::Table table(&ctx->backend);

    /* Make sure the ToC exactly fits in the space allocated for it */
    assert(table.capacity() == TOC_PART_SIZE);

    rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    rc = mbox_command_dispatch(ctx, create_read_window,
                               sizeof(create_read_window));
    assert(rc == 1);

    rc = mbox_cmp(ctx, response, sizeof(response));
    assert(rc == 0);

    /* Ensure our partition table is present and as expected */
    const pnor_partition_table& toc = table.getHostTable();
    rc = memcmp(ctx->mem, &toc, table.size());
    assert(rc == 0);

    return 0;
}
