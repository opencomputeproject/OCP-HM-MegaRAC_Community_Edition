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

#ifndef TORDUMP_H
#define TORDUMP_H

#include "crashdump.h"

/******************************************************************************
 *
 *   Common Defines
 *
 ******************************************************************************/
#define TD_JSON_STRING_LEN 66
#define TD_JSON_CHA_NAME "cha%d"
#define TD_JSON_TOR_NAME "index%d"
#define TD_JSON_SUBINDEX_NAME "subindex%d"
#define TD_NA "N/A"
#define TD_UA "UA:0x%x"
#define TD_DF "DF:0x%x"
#define TD_UA_DF "UA:0x%x,DF:0x%x"
#define TD_DATA_CC_RC ",CC:0x%x,RC:0x%x"
#define TD_FIXED_DATA_CC_RC "0x0,CC:0x%x,RC:0x%x"
#define SIZE_FAILURE 7

/******************************************************************************
 *
 *   ICX1 Defines
 *
 ******************************************************************************/
#define TD_TORS_PER_CHA_ICX1 32
#define TD_SUBINDEX_PER_TOR_ICX1 8

typedef struct
{
    Model cpuModel;
    int (*logTorDumpVx)(CPUInfo* cpuInfo, cJSON* pJsonChild);
} STorDumpVx;

int logTorDump(CPUInfo* cpuInfo, cJSON* pJsonChild);

#endif // TORDUMP_H
