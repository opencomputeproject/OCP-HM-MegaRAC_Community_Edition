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
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syslog.h>
#include <unistd.h>

#include <cassert>
#include <experimental/filesystem>

static constexpr auto BLOCK_SIZE = 0x1000;
static constexpr auto DATA_SIZE = 8;

const uint8_t data[DATA_SIZE] = {0xa0, 0xa1, 0xa2, 0xa3,
                                 0xa4, 0xa5, 0xa6, 0xa7};

const std::string toc[] = {
    "partition01=TEST1,00001000,00002000,80,ECC,READWRITE",
};

int main(void)
{
    namespace fs = std::experimental::filesystem;
    namespace test = openpower::virtual_pnor::test;

    struct mbox_context _ctx, *ctx = &_ctx;
    char src[DATA_SIZE]{0};
    void* map;
    int rc;
    int fd;

    /* Setup */
    memset(ctx, 0, sizeof(mbox_context));

    mbox_vlog = &mbox_log_console;
    verbosity = (verbose)2;

    ctx->backend.flash_size = 0x2000;
    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);
    root.write("TEST1", data, sizeof(data));
    /* flash_write doesn't copy the file for us */
    assert(fs::copy_file(root.ro() / "TEST1", root.rw() / "TEST1"));
    fs::path patch = root.patch() / "TEST1";
    assert(fs::copy_file(root.ro() / "TEST1", patch));

    /* Test */
    memset(src, 0x33, sizeof(src));
    rc = backend_write(&ctx->backend, 0x1000, src, sizeof(src));
    assert(rc == 0);

    /* Check that RW file is unmodified after the patch write */
    fd = open((root.rw() / "TEST1").c_str(), O_RDONLY);
    map = mmap(NULL, sizeof(src), PROT_READ, MAP_SHARED, fd, 0);
    assert(map != MAP_FAILED);
    rc = memcmp(data, map, sizeof(src));
    assert(rc == 0);
    munmap(map, sizeof(src));
    close(fd);

    /* Check that PATCH is modified with the new data */
    fd = open(patch.c_str(), O_RDONLY);
    map = mmap(NULL, sizeof(src), PROT_READ, MAP_SHARED, fd, 0);
    assert(map != MAP_FAILED);
    rc = memcmp(src, map, sizeof(src));
    assert(rc == 0);
    munmap(map, sizeof(src));
    close(fd);

    return rc;
}
