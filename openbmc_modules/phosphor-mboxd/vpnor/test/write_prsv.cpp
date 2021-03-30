// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "mbox.h"
#include "mboxd_flash.h"

#include "vpnor/test/tmpd.hpp"

static constexpr auto BLOCK_SIZE = 0x1000;

const std::string toc[] = {
    "partition01=TEST1,00001000,00002000,80,ECC,PRESERVED",
};

namespace test = openpower::virtual_pnor::test;

int main(void)
{
    namespace fs = std::experimental::filesystem;

    struct mbox_context _ctx, *ctx = &_ctx;
    uint8_t src[8];
    void *map;
    int fd;
    int rc;

    /* Setup */
    memset(ctx, 0, sizeof(mbox_context));

    mbox_vlog = &mbox_log_console;
    verbosity = (verbose)2;

    test::VpnorRoot root(ctx, toc, BLOCK_SIZE);
    init_vpnor_from_paths(ctx);

    /* Test */
    memset(src, 0xaa, sizeof(src));
    rc = write_flash(ctx, 0x1000, src, sizeof(src));
    assert(rc == 0);

    /* Verify */
    fd = open((root.prsv() / "TEST1").c_str(), O_RDONLY);
    assert(fd >= 0);
    map = mmap(NULL, sizeof(src), PROT_READ, MAP_SHARED, fd, 0);
    assert(map != MAP_FAILED);

    rc = memcmp(src, map, sizeof(src));
    assert(rc == 0);
    munmap(map, sizeof(src));
    close(fd);

    /* Cleanup */
    destroy_vpnor(ctx);

    return 0;
}
