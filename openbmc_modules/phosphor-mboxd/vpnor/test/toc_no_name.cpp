// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "config.h"
#include <assert.h>

#include "vpnor/pnor_partition_table.hpp"

static constexpr auto BLOCK_SIZE = 4 * 1024;

int main()
{
    namespace vpnor = openpower::virtual_pnor;

    const std::string line = "partition01=,00000000,00000400,80,ECC,PRESERVED";
    struct pnor_partition part;

    try
    {
        vpnor::parseTocLine(line, BLOCK_SIZE, part);
    }
    catch (vpnor::MalformedTocEntry& e)
    {
        return 0;
    }

    assert(false);
}
