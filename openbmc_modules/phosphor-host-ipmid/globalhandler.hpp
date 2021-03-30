#pragma once

#include <stdint.h>

// Various GLOBAL operations under a single command.
enum ipmi_global_control_cmds : uint8_t
{
    IPMI_CMD_COLD_RESET = 0x02,
};
