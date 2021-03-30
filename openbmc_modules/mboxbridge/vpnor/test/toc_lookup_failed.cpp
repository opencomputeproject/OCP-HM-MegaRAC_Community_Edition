// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include "config.h"

extern "C" {
#include "backend.h"
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/table.hpp"
#include "vpnor/test/tmpd.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <cassert>
#include <cstring>

static constexpr auto BLOCK_SIZE = 0x1000;
static constexpr auto ERASE_SIZE = BLOCK_SIZE;
static constexpr auto PNOR_SIZE = 64 * 1024 * 1024;
static constexpr auto MEM_SIZE = 32 * 1024 * 1024;
static constexpr auto N_WINDOWS = 1;
static constexpr auto WINDOW_SIZE = BLOCK_SIZE * 2;

const std::string toc[] = {
    "partition01=ONE,00001000,00002000,80,",
};

int main()
{
    namespace err = sdbusplus::xyz::openbmc_project::Common::Error;
    namespace test = openpower::virtual_pnor::test;
    namespace vpnor = openpower::virtual_pnor;

    struct mbox_context* ctx;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

    ctx = mbox_create_frontend_context(N_WINDOWS, WINDOW_SIZE);

    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);
    vpnor::partition::Table table(&ctx->backend);

    try
    {
        table.partition("TWO");
    }
    catch (vpnor::UnknownPartition& e)
    {
        return 0;
    }

    assert(false);
}
