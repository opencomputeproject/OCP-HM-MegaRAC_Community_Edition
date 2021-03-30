// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "config.h"

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

#include <cassert>
#include <experimental/filesystem>

#include "vpnor/backend.h"

static const auto BLOCK_SIZE = 4096;
static const auto ERASE_SIZE = BLOCK_SIZE;
static const auto WINDOW_SIZE = 2 * BLOCK_SIZE;
static const auto MEM_SIZE = WINDOW_SIZE;
static const auto N_WINDOWS = 1;
static const auto PNOR_SIZE = 4 * BLOCK_SIZE;

const std::string toc[] = {
    "partition01=ONE,00001000,00002000,80,ECC,READONLY",
    "partition02=TWO,00002000,00004000,80,ECC,READONLY",
};

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

static const uint8_t request_one[] = {0x04, 0x01, 0x01, 0x00, 0x02, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00};

static const uint8_t response_one[] = {0x04, 0x01, 0xfe, 0xff, 0x01,
                                       0x00, 0x01, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x01};

static const uint8_t request_two[] = {0x04, 0x02, 0x02, 0x00, 0x02, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00};

static const uint8_t response_two[] = {0x04, 0x02, 0xfe, 0xff, 0x02,
                                       0x00, 0x02, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x01};

namespace test = openpower::virtual_pnor::test;

int main()
{
    struct mbox_context* ctx;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(PNOR_SIZE, ERASE_SIZE);

    ctx = mbox_create_frontend_context(N_WINDOWS, WINDOW_SIZE);
    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);

    int rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    rc = mbox_command_dispatch(ctx, request_one, sizeof(request_one));
    assert(rc == 1);

    rc = mbox_cmp(ctx, response_one, sizeof(response_one));
    assert(rc == 0);

    rc = mbox_command_dispatch(ctx, request_two, sizeof(request_two));
    assert(rc == 1);

    rc = mbox_cmp(ctx, response_two, sizeof(response_two));
    assert(rc == 0);

    return rc;
}
