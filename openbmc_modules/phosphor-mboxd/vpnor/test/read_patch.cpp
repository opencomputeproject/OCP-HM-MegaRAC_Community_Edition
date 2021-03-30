// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "config.h"
#include "common.h"
#include "vpnor/mboxd_pnor_partition_table.h"

#include <assert.h>

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

static constexpr auto BLOCK_SIZE = 0x1000;
static constexpr auto ERASE_SIZE = BLOCK_SIZE;
static constexpr auto PART_SIZE = BLOCK_SIZE * 4;
static constexpr auto PATCH_SIZE = PART_SIZE / 2;
static constexpr auto N_WINDOWS = 2;
static constexpr auto WINDOW_SIZE = PART_SIZE * 8;
static constexpr auto MEM_SIZE = WINDOW_SIZE * N_WINDOWS;
static constexpr auto PNOR_SIZE = MEM_SIZE * 2;

const std::string toc[] = {
    "partition01=ONE,00001000,00005000,80,ECC,READONLY",
};

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

static const uint8_t create_read_window[] = {0x04, 0x01, 0x01, 0x00, 0x04, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00};

static const uint8_t response[] = {0x04, 0x01, 0xc0, 0xff, 0x04, 0x00, 0x01,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

int main()
{
    namespace test = openpower::virtual_pnor::test;

    struct mbox_context *ctx;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(PNOR_SIZE, ERASE_SIZE);
    ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);
    test::VpnorRoot root(ctx, toc, BLOCK_SIZE);

    // PATCH_SIZE is smaller than the size of the partition we defined. This
    // test ensures that mboxd will behave correctly when we request an offset
    // that is beyond the size of the backing file, but is in the set of valid
    // offsets for the partition as defined by the ToC.
    std::vector<uint8_t> patch(PATCH_SIZE, 0xff);
    root.patch("ONE", patch.data(), patch.size());

    init_vpnor_from_paths(ctx);

    int rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    rc = mbox_command_dispatch(ctx, create_read_window,
                               sizeof(create_read_window));
    assert(rc == 1);

    rc = mbox_cmp(ctx, response, sizeof(response));
    assert(rc == 0);

    return 0;
}
