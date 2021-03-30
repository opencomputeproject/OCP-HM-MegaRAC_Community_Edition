// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "config.h"

#include "vpnor/table.hpp"

#include <cassert>

static constexpr auto BLOCK_SIZE = 4 * 1024;

int main()
{
    namespace vpnor = openpower::virtual_pnor;

    struct pnor_partition part;
    std::string line;

    line = "partition01=FOO,00001000,,80,ECC,PRESERVED";
    try
    {
        openpower::virtual_pnor::parseTocLine(line, BLOCK_SIZE, part);
    }
    catch (vpnor::MalformedTocEntry& e)
    {
        return 0;
    }

    assert(false);
}
