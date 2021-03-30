// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "config.h"
#include <assert.h>

#include "common.h"
#include "vpnor/pnor_partition_defs.h"
#include "vpnor/pnor_partition_table.hpp"

static constexpr auto BLOCK_SIZE = 4 * 1024;
static constexpr auto DATA_MASK = ((1 << 24) - 1);

int main()
{
    namespace vpnor = openpower::virtual_pnor;

    struct pnor_partition part;
    std::string line;

    mbox_vlog = mbox_log_console;
    verbosity = MBOX_LOG_DEBUG;

    line = "partition01=FOO,00001000,00002000,80,ECC";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert((part.data.user.data[0]) == PARTITION_ECC_PROTECTED);
    assert(!(part.data.user.data[1] & DATA_MASK));

    line = "partition01=FOO,00001000,00002000,80,PRESERVED";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert(!(part.data.user.data[0]));
    assert((part.data.user.data[1] & DATA_MASK) == PARTITION_PRESERVED);

    line = "partition01=FOO,00001000,00002000,80,READONLY";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert(!(part.data.user.data[0]));
    assert((part.data.user.data[1] & DATA_MASK) == PARTITION_READONLY);

    /* BACKUP is unimplemented */
    line = "partition01=FOO,00001000,00002000,80,BACKUP";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert(!(part.data.user.data[0]));
    assert(!(part.data.user.data[1] & DATA_MASK));

    line = "partition01=FOO,00001000,00002000,80,REPROVISION";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert(!(part.data.user.data[0]));
    assert((part.data.user.data[1] & DATA_MASK) == PARTITION_REPROVISION);

    line = "partition01=FOO,00001000,00002000,80,VOLATILE";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert(!(part.data.user.data[0]));
    assert((part.data.user.data[1] & DATA_MASK) == PARTITION_VOLATILE);

    line = "partition01=FOO,00001000,00002000,80,CLEARECC";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert(!(part.data.user.data[0]));
    assert((part.data.user.data[1] & DATA_MASK) == PARTITION_CLEARECC);

    line = "partition01=FOO,00001000,00002000,80,READWRITE";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert(!(part.data.user.data[0]));
    assert(((part.data.user.data[1] & DATA_MASK) ^ PARTITION_READONLY) ==
           PARTITION_READONLY);

    line = "partition01=FOO,00001000,00002000,80,";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert(!(part.data.user.data[0]));
    assert(!(part.data.user.data[1] & DATA_MASK));

    line = "partition01=FOO,00001000,00002000,80,junk";
    vpnor::parseTocLine(line, BLOCK_SIZE, part);
    assert(!(part.data.user.data[0]));
    assert(!(part.data.user.data[1] & DATA_MASK));

    return 0;
}
