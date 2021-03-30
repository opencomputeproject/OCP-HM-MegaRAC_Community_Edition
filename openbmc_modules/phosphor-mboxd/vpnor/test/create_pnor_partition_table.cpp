// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include <assert.h>
#include <string.h>

#include "config.h"
#include "vpnor/pnor_partition_table.hpp"

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

static const auto BLOCK_SIZE = 4 * 1024;
static const auto ERASE_SIZE = BLOCK_SIZE;
static const auto PNOR_SIZE = 64 * 1024 * 1024;
static const auto MEM_SIZE = PNOR_SIZE;
static const auto N_WINDOWS = 1;
static const auto WINDOW_SIZE = BLOCK_SIZE;

const std::string toc[] = {
    "partition01=HBB,00000000,00001000,80,ECC,PRESERVED",
};
constexpr auto partitionName = "HBB";

namespace test = openpower::virtual_pnor::test;

int main()
{
    struct mbox_context* ctx;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(PNOR_SIZE, ERASE_SIZE);
    ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);
    test::VpnorRoot root(ctx, toc, BLOCK_SIZE);
    const openpower::virtual_pnor::partition::Table table(ctx);

    pnor_partition_table expectedTable{};
    expectedTable.data.magic = PARTITION_HEADER_MAGIC;
    expectedTable.data.version = PARTITION_VERSION_1;
    expectedTable.data.size = 1;
    expectedTable.data.entry_size = sizeof(pnor_partition);
    expectedTable.data.entry_count = 1;
    expectedTable.data.block_size = BLOCK_SIZE;
    expectedTable.data.block_count =
        (PNOR_SIZE) / expectedTable.data.block_size;
    expectedTable.checksum =
        openpower::virtual_pnor::details::checksum(expectedTable.data);

    pnor_partition expectedPartition{};
    strcpy(expectedPartition.data.name, partitionName);
    expectedPartition.data.base = 0;
    expectedPartition.data.size = 1;
    expectedPartition.data.actual = 0x1000;
    expectedPartition.data.id = 1;
    expectedPartition.data.pid = PARENT_PATITION_ID;
    expectedPartition.data.type = PARTITION_TYPE_DATA;
    expectedPartition.data.flags = 0;
    expectedPartition.data.user.data[0] = PARTITION_ECC_PROTECTED;
    expectedPartition.data.user.data[1] |= PARTITION_PRESERVED;
    expectedPartition.data.user.data[1] |= PARTITION_VERSION_CHECK_SHA512;
    expectedPartition.checksum =
        openpower::virtual_pnor::details::checksum(expectedPartition.data);

    const pnor_partition_table& result = table.getNativeTable();

    auto rc = memcmp(&expectedTable, &result, sizeof(pnor_partition_table));
    assert(rc == 0);

    rc = memcmp(&expectedPartition, &result.partitions[0],
                sizeof(pnor_partition));
    assert(rc == 0);

    const pnor_partition& first = table.partition(0);
    rc = memcmp(&first, &result.partitions[0], sizeof(pnor_partition));
    assert(rc == 0);

    return 0;
}
