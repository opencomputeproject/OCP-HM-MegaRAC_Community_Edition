// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "config.h"

#include <assert.h>
#include <string.h>

#include "vpnor/mboxd_pnor_partition_table.h"

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

const std::string toc[] = {
    "partition01=ONE,00001000,00002000,80,ECC,READONLY",
    "partition02=TWO,00002000,00003000,80,ECC,READONLY",
};

uint8_t data[8] = {0xaa, 0x55, 0xaa, 0x66, 0x77, 0x88, 0x99, 0xab};

static constexpr auto BLOCK_SIZE = 0x1000;
static constexpr auto MEM_SIZE = BLOCK_SIZE * 2;
static constexpr auto ERASE_SIZE = BLOCK_SIZE;
static constexpr auto N_WINDOWS = 1;
static constexpr auto WINDOW_SIZE = MEM_SIZE;
static constexpr auto PNOR_SIZE = MEM_SIZE * 2;

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

static const uint8_t create_read_window[] = {0x04, 0x01, 0x01, 0x00, 0x02, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00};

static const uint8_t response[] = {
    0x04, 0x01, 0xfe, 0xff, 0x01, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
};

int main()
{
    namespace test = openpower::virtual_pnor::test;
    namespace fs = std::experimental::filesystem;

    struct mbox_context *ctx;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(PNOR_SIZE, ERASE_SIZE);

    ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);
    test::VpnorRoot root(ctx, toc, BLOCK_SIZE);
    init_vpnor_from_paths(ctx);

    int rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    // Request a read window that would cover both partitions. With the current
    // behaviour, we expect to receive a reply describing a window that covers
    // the first partition but is limited in size to exclude the second
    // partition.
    rc = mbox_command_dispatch(ctx, create_read_window,
                               sizeof(create_read_window));
    assert(rc == 1);

    rc = mbox_cmp(ctx, response, sizeof(response));
    assert(rc == 0);

    return 0;
}
