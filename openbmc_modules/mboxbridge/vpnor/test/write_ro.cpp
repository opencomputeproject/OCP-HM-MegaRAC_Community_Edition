// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include "config.h"

extern "C" {
#include "backend.h"
#include "common.h"
#include "mboxd.h"
}

#include "vpnor/test/tmpd.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

static constexpr auto BLOCK_SIZE = 0x1000;

const std::string toc[] = {
    "partition01=TEST1,00001000,00002000,80,ECC,READONLY",
};

int main(void)
{
    namespace fs = std::experimental::filesystem;
    namespace test = openpower::virtual_pnor::test;

    struct mbox_context _ctx, *ctx = &_ctx;
    uint8_t src[8] = {0};
    int rc;

    /* Setup */
    memset(ctx, 0, sizeof(mbox_context));

    mbox_vlog = &mbox_log_console;
    verbosity = (verbose)2;

    ctx->backend.flash_size = 0x2000;
    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);

    /* Test */
    rc = backend_write(&ctx->backend, 0x1000, src, sizeof(src));

    /* Verify we can't write to RO partitions */
    assert(rc != 0);

    return 0;
}
