/******************************************************************************
 *
 * INTEL CONFIDENTIAL
 *
 * Copyright 2019 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with no express
 * or implied warranties, other than those that are expressly stated in the
 * License.
 *
 ******************************************************************************/

#pragma once
#include "utils_dbusplus.hpp"

#include <linux/peci-ioctl.h>
#include <peci.h>

#include <array>
#include <vector>

extern "C" {
#include <cjson/cJSON.h>

#include "CrashdumpSections/crashdump.h"
#include "safe_str_lib.h"
}
#define ICXD_MODEL 0x606C0

namespace crashdump
{
constexpr char const* dbgStatusItemName = "status";
constexpr const char* dbgFailedStatus = "N/A";

void setResetDetected();
} // namespace crashdump
