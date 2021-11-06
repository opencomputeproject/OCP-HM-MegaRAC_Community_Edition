/******************************************************************************
 *
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
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
#include "crashdump.hpp"

#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <variant>

namespace crashdump
{
int getBMCVersionDBus(char* bmcVerStr, size_t bmcVerStrSize);
int getBIOSVersionDBus(char* biosVerStr, size_t biosVerStrSize);
std::shared_ptr<sdbusplus::bus::match::match>
    startHostStateMonitor(std::shared_ptr<sdbusplus::asio::connection> conn);
} // namespace crashdump

int logSysInfoCommon(cJSON* pJsonChild);