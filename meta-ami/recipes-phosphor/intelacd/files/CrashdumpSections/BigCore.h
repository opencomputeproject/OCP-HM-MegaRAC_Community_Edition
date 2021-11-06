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

#ifndef BIGCORE_H
#define BIGCORE_H

#include "crashdump.h"

/******************************************************************************
 *
 *   Common Defines
 *
 ******************************************************************************/
#define CD_REG_NAME_LEN 32

#define CD_JSON_STRING_LEN 256
#define CD_JSON_THREAD_NAME "thread%d"
#define CD_JSON_THREAD_1 "thread1"
#define CD_JSON_UA "UA:0x%x"
#define CD_JSON_DF "DF:0x%x"
#define CD_JSON_UA_DF "UA:0x%x,DF:0x%x"
#define CD_JSON_FIXED_DATA_CC_RC "0x0,CC:0x%x,RC:0x%x"
#define CD_JSON_DATA_CC_RC ",CC:0x%x,RC:0x%x"
#define SIZE_FAILURE 7

/******************************************************************************
 *
 *   ICX1 Defines
 *
 ******************************************************************************/
#define CD_MT_THREADS_PER_CORE 2
#define CD_ST_THREADS_PER_CORE 1

#define CD_LBRS_PER_CORE 32
#define CD_ENTRIES_PER_LBR 3
#define CD_LBR_SIZE 8

#define CD_JSON_NUM_SQ_ENTRIES 64
#define CD_JSON_NUM_SPR_SQD_ENTRIES 48
#define CD_JSON_NUM_SPR_SQS_ENTRIES 48
#define CD_JSON_SQ_ENTRY_SIZE 8

#define CD_JSON_CORE_NAME "core%d"

#define CD_JSON_LBR_NAME_FROM "LBR%d_FROM"
#define CD_JSON_LBR_NAME_TO "LBR%d_TO"
#define CD_JSON_LBR_NAME_INFO "LBR%d_INFO"

#define CD_JSON_SQ_ENTRY_NAME "entry%d"
#define CRASHDUMP_MAX_SIZE 0x7f8

enum BigCoreParams
{
    bigCoreRegName,
    bigCoreRegSize
};

/******************************************************************************
 *
 *   ICX1 Structures
 *
 ******************************************************************************/
typedef struct
{
    char* name;
    uint8_t size;
} SCrashdumpRegICX1;

typedef union
{
    uint64_t raw;
    struct
    {
        uint32_t version;
        uint32_t regDumpSize : 16;
        uint32_t sqDumpSize : 16;
    } field;
} UCrashdumpVerSize;

typedef union
{
    uint64_t raw;
    struct
    {
        uint32_t whoami;
        uint32_t multithread : 1;
        uint32_t sgxMask : 1;
        uint32_t reserved : 30;
    } field;
} UCrashdumpWhoMisc;
#define CD_WHO_MISC_OFFSET 3

typedef struct
{
    Model cpuModel;
    int (*logCrashdumpVx)(CPUInfo* cpuInfo, cJSON* pJsonChild);
} SCrashdumpVx;

int logCrashdump(CPUInfo* cpuInfo, cJSON* pJsonChild);

#define ICX_A0_FRAME_BYTE_OFFSET 32
#define ICX_A0_CRASHDUMP_DISABLED 1
#define ICX_A0_CRASHDUMP_ENABLED 0
#define SIZE_OF_0x0 3

#endif // BIGCORE_H
