// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <experimental/filesystem>
#include <fstream>
#include <string.h>
#include <vector>

#include "config.h"
#include "vpnor/mboxd_pnor_partition_table.h"

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

static const auto BLOCK_SIZE = 4096;
static const auto ERASE_SIZE = BLOCK_SIZE;
static const auto WINDOW_SIZE = 16 * BLOCK_SIZE;
static const auto N_WINDOWS = 2;
static const auto MEM_SIZE = N_WINDOWS * WINDOW_SIZE;

const std::string toc[] = {
    "partition01=ONE,00001000,00011000,80,ECC,READONLY",
};

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

namespace test = openpower::virtual_pnor::test;

int main()
{
    uint8_t request[] = {0x04, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t response[] = {0x04, 0x01, 0xe0, 0xff, 0x10, 0x00, 0x01,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

    struct mbox_context *ctx;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

    ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);
    test::VpnorRoot root(ctx, toc, BLOCK_SIZE);
    init_vpnor_from_paths(ctx);

    int rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    for (int i = 1; i < (0x10000 / BLOCK_SIZE); i++)
    {
        /* Update the requested offset */
        put_u16(&request[2], i);

        /*
         * Reuse the offset as a sequence number, because it's unique. Request
         * and response have the same sequence number
         */
        request[1] = i;
        response[1] = i;

        rc = mbox_command_dispatch(ctx, request, sizeof(request));
        assert(rc == 1);

        /* Check that it maps to the same window each time */
        rc = mbox_cmp(ctx, response, sizeof(response));
        assert(rc == 0);
    }

    return 0;
}
