/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"

#include "ethstats.hpp"

#include <cstdio>
#include <ipmid/iana.hpp>
#include <ipmid/oemopenbmc.hpp>
#include <ipmid/oemrouter.hpp>

namespace ethstats
{

EthStats handler;

static ipmi_ret_t ethStatCommand(ipmi_cmd_t cmd __attribute__((unused)),
                                 const uint8_t* reqBuf, uint8_t* replyCmdBuf,
                                 size_t* dataLen)
{
    return handleEthStatCommand(reqBuf, replyCmdBuf, dataLen, &handler);
}

} // namespace ethstats

void setupGlobalOemEthStats() __attribute__((constructor));

void setupGlobalOemEthStats()
{
    oem::Router* oemRouter = oem::mutableRouter();

#if ENABLE_GOOGLE
    /* Install in Google OEM Namespace when enabled. */
    std::fprintf(stderr,
                 "Registering OEM:[%#08X], Cmd:[%#04X] for Ethstats Commands\n",
                 oem::googOemNumber, oem::Cmd::ethStatsCmd);

    oemRouter->registerHandler(oem::googOemNumber, oem::Cmd::ethStatsCmd,
                               ethstats::ethStatCommand);
#endif

    std::fprintf(stderr,
                 "Registering OEM:[%#08X], Cmd:[%#04X] for Ethstats Commands\n",
                 oem::obmcOemNumber, oem::Cmd::ethStatsCmd);

    oemRouter->registerHandler(oem::obmcOemNumber, oem::Cmd::ethStatsCmd,
                               ethstats::ethStatCommand);
}
