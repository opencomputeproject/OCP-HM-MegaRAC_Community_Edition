// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "config.h"

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

#include <cassert>
#include <cstring>
#include <experimental/filesystem>
#include <fstream>
#include <vector>

#include "vpnor/backend.h"

// A read window assumes that the toc is located at offset 0,
// so create dummy partition at arbitrary offset 0x1000.
const std::string toc[] = {
    "partition01=HBB,00001000,00002000,80,ECC,READONLY",
    "partition05=DJVPD,0x000e5000,0x0022d000,00,ECC,PRESERVED",
};

static const auto BLOCK_SIZE = 4096;
static const auto ERASE_SIZE = BLOCK_SIZE;
static const auto N_WINDOWS = 1;
static const auto WINDOW_SIZE = 1024 * 1024;
static const auto MEM_SIZE = WINDOW_SIZE * 3;

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

// Used to check the alignment
static const uint8_t create_read_window1[] = {
    0x04, 0x01, 0xe7, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t response1[] = {0x04, 0x01, 0x00, 0xfd, 0x00, 0x01, 0xe5,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

static const uint8_t create_read_window2[] = {
    0x04, 0x02, 0xe5, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t response2[] = {0x04, 0x02, 0x00, 0xfd, 0x48, 0x00, 0xe5,
                                    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

namespace test = openpower::virtual_pnor::test;

int main()
{
    struct mbox_context* ctx;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

    ctx = mbox_create_frontend_context(N_WINDOWS, WINDOW_SIZE);

    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);

    int rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    // send the request for partition5
    rc = mbox_command_dispatch(ctx, create_read_window1,
                               sizeof(create_read_window1));
    assert(rc == 1);
    rc = mbox_cmp(ctx, response1, sizeof(response1));
    assert(rc == 0);

    // send the request for partition5
    rc = mbox_command_dispatch(ctx, create_read_window2,
                               sizeof(create_read_window2));
    assert(rc == 1);
    rc = mbox_cmp(ctx, response2, sizeof(response2));
    assert(rc == 0);

    return rc;
}
