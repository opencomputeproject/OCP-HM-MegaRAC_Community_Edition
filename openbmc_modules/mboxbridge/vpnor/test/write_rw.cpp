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
    "partition01=TEST1,00001000,00002000,80,ECC,READWRITE",
};

int main(void)
{
    namespace fs = std::experimental::filesystem;
    namespace test = openpower::virtual_pnor::test;

    struct mbox_context _ctx, *ctx = &_ctx;
    uint8_t src[8] = {0};
    void* map;
    int rc;
    int fd;

    /* Setup */
    memset(ctx, 0, sizeof(mbox_context));

    mbox_vlog = &mbox_log_console;
    verbosity = (verbose)2;

    ctx->backend.flash_size = 0x2000;
    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);
    /* flash_write() doesn't copy the file for us */
    assert(fs::copy_file(root.ro() / "TEST1", root.rw() / "TEST1"));

    /* Test */
    memset(src, 0xbb, sizeof(src));
    rc = backend_write(&ctx->backend, 0x1000, src, sizeof(src));
    assert(rc == 0);
    fd = open((root.rw() / "TEST1").c_str(), O_RDONLY);
    map = mmap(NULL, sizeof(src), PROT_READ, MAP_SHARED, fd, 0);
    assert(map != MAP_FAILED);
    rc = memcmp(src, map, sizeof(src));
    assert(rc == 0);

    /* Ensure single byte writes function */
    memset(src, 0xcc, sizeof(src));
    rc = backend_write(&ctx->backend, 0x1000, src, sizeof(src));
    assert(rc == 0);
    rc = memcmp(src, map, sizeof(src));
    assert(rc == 0);

    src[0] = 0xff;
    rc = backend_write(&ctx->backend, 0x1000, src, 1);
    assert(rc == 0);
    rc = memcmp(src, map, sizeof(src));
    assert(rc == 0);

    src[1] = 0xff;
    rc = backend_write(&ctx->backend, 0x1000 + 1, &src[1], 1);
    assert(rc == 0);
    rc = memcmp(src, map, sizeof(src));
    assert(rc == 0);

    src[2] = 0xff;
    rc = backend_write(&ctx->backend, 0x1000 + 2, &src[2], 1);
    assert(rc == 0);
    rc = memcmp(src, map, sizeof(src));
    assert(rc == 0);

    /* Writes past the end of the partition should fail */
    rc = backend_write(&ctx->backend, 0x1000 + 0xff9, src, sizeof(src));
    assert(rc < 0);

    /* Check that RW file is unmodified after the bad write */
    fd = open((root.rw() / "TEST1").c_str(), O_RDONLY);
    map = mmap(NULL, sizeof(src), PROT_READ, MAP_SHARED, fd, 0);
    assert(map != MAP_FAILED);
    rc = memcmp(src, map, sizeof(src));
    assert(rc == 0);

    munmap(map, sizeof(src));
    close(fd);

    return 0;
}
