#pragma once

#include <cstdint>

// IPMI commands for Storage net functions.
enum ipmi_netfn_storage_cmds
{
    // Get capability bits
    IPMI_CMD_GET_FRU_INV_AREA_INFO = 0x10,
    IPMI_CMD_GET_REPOSITORY_INFO = 0x20,
    IPMI_CMD_READ_FRU_DATA = 0x11,
    IPMI_CMD_RESERVE_SDR = 0x22,
    IPMI_CMD_GET_SDR = 0x23,
    IPMI_CMD_GET_SEL_INFO = 0x40,
    IPMI_CMD_RESERVE_SEL = 0x42,
    IPMI_CMD_GET_SEL_ENTRY = 0x43,
    IPMI_CMD_ADD_SEL = 0x44,
    IPMI_CMD_DELETE_SEL = 0x46,
    IPMI_CMD_CLEAR_SEL = 0x47,
    IPMI_CMD_GET_SEL_TIME = 0x48,
    IPMI_CMD_SET_SEL_TIME = 0x49,

};
