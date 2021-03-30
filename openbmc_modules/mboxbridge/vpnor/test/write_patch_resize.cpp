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
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syslog.h>
#include <unistd.h>

#include <cassert>
#include <experimental/filesystem>

static constexpr auto BLOCK_SIZE = 0x1000;
static constexpr auto PART_SIZE = BLOCK_SIZE;
static constexpr auto PATCH_SIZE = BLOCK_SIZE / 2;
static constexpr auto UPDATE_SIZE = BLOCK_SIZE;

const std::string toc[] = {
    "partition01=TEST1,00001000,00002000,80,ECC,READWRITE",
};

int main(void)
{
    namespace fs = std::experimental::filesystem;
    namespace test = openpower::virtual_pnor::test;

    struct mbox_context _ctx, *ctx = &_ctx;
    void* map;
    int rc;
    int fd;

    /* Setup */
    memset(ctx, 0, sizeof(mbox_context));

    mbox_vlog = &mbox_log_console;
    verbosity = (verbose)2;

    ctx->backend.flash_size = 0x2000;
    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);
    std::vector<uint8_t> roContent(PART_SIZE, 0xff);
    root.write("TEST1", roContent.data(), roContent.size());
    /* flash_write doesn't copy the file for us */
    std::vector<uint8_t> patchContent(PATCH_SIZE, 0xaa);
    root.patch("TEST1", patchContent.data(), patchContent.size());

    /* Test */
    std::vector<uint8_t> update(UPDATE_SIZE, 0x55);
    rc = backend_write(&ctx->backend, 0x1000, update.data(), update.size());
    assert(rc == 0);

    /* Check that PATCH is modified with the new data */
    fs::path patch = root.patch() / "TEST1";
    assert(UPDATE_SIZE == fs::file_size(patch));
    fd = open(patch.c_str(), O_RDONLY);
    map = mmap(NULL, UPDATE_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    assert(map != MAP_FAILED);
    rc = memcmp(update.data(), map, update.size());
    assert(rc == 0);
    munmap(map, update.size());
    close(fd);

    return rc;
}
