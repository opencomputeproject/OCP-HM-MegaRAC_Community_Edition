#pragma once

#include <stdint.h>

#include <cstddef>

// IPMI commands for Chassis net functions.
enum ipmi_netfn_chassis_cmds
{
    IPMI_CMD_GET_CHASSIS_CAP = 0x00,
    // Chassis Status
    IPMI_CMD_CHASSIS_STATUS = 0x01,
    // Chassis Control
    IPMI_CMD_CHASSIS_CONTROL = 0x02,
    IPMI_CMD_CHASSIS_IDENTIFY = 0x04,
    IPMI_CMD_SET_CHASSIS_CAP = 0x05,
    // Get capability bits
    IPMI_CMD_SET_SYS_BOOT_OPTIONS = 0x08,
    IPMI_CMD_GET_SYS_BOOT_OPTIONS = 0x09,
    IPMI_CMD_GET_POH_COUNTER = 0x0F,
};

// Command specific completion codes
enum ipmi_chassis_return_codes
{
    IPMI_OK = 0x0,
    IPMI_CC_PARM_NOT_SUPPORTED = 0x80,
};

// Generic completion codes,
// see IPMI doc section 5.2
enum ipmi_generic_return_codes
{
    IPMI_OUT_OF_SPACE = 0xC4,
};

// Various Chassis operations under a single command.
enum ipmi_chassis_control_cmds : uint8_t
{
    CMD_POWER_OFF = 0x00,
    CMD_POWER_ON = 0x01,
    CMD_POWER_CYCLE = 0x02,
    CMD_HARD_RESET = 0x03,
    CMD_PULSE_DIAGNOSTIC_INTR = 0x04,
    CMD_SOFT_OFF_VIA_OVER_TEMP = 0x05,
};
enum class BootOptionParameter : size_t
{
    BOOT_INFO = 0x4,
    BOOT_FLAGS = 0x5,
    OPAL_NETWORK_SETTINGS = 0x61
};

enum class BootOptionResponseSize : size_t
{
    BOOT_FLAGS = 5,
    OPAL_NETWORK_SETTINGS = 50
};

enum class ChassisIDState : uint8_t
{
    off = 0x0,
    temporaryOn = 0x1,
    indefiniteOn = 0x2,
    reserved = 0x3
};
