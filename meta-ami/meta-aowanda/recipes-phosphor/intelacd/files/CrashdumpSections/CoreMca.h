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

#ifndef COREMCA_H
#define COREMCA_H

#include "crashdump.h"

/******************************************************************************
 *
 *   Common Defines
 *
 ******************************************************************************/
#define CORE_MCA_JSON_STRING_LEN 64
#define CORE_MCA_JSON_CORE_NAME "core%d"
#define CORE_MCA_JSON_THREAD_NAME "thread%d"

#define CORE_MCA_NA "N/A"
#define CORE_MCA_UA "UA:0x%x"
#define CORE_MCA_DF "DF:0x%x"
#define CORE_MCA_UA_DF "UA:0x%x,DF:0x%x"
#define CORE_MCA_DATA_CC_RC "0x%" PRIx64 ",CC:0x%x,RC:0x%x"
#define CORE_MCA_FIXED_DATA_CC_RC "0x0,CC:0x%x,RC:0x%x"
#define CORE_MCA_UINT64_FMT "0x%" PRIx64 ""
#define CORE_MCA_VALID true

#define FILE_MCA_KEY "_input_file_mca"
#define FILE_MCA_ERR "Error parsing MCA core section"

/******************************************************************************
 *
 *   ICX/SPR Defines
 *
 ******************************************************************************/
#define CORE_MCA_THREADS_PER_CORE 2

/******************************************************************************
 *
 *   ICX/SPR Structures
 *
 ******************************************************************************/
enum MCA_CORE
{
    MCA_CORE_BANK_NAME,
    MCA_CORE_REG_NAME,
    MCA_CORE_ADDR,
};

typedef struct
{
    char* bankName;
    char* regName;
    uint16_t addr;
} SCoreMcaReg;

typedef struct
{
    Model cpuModel;
    int (*logCoreMcaVx)(CPUInfo* cpuInfo, cJSON* pJsonChild);
} SCoreMcaLogVx;

int logCoreMca(CPUInfo* cpuInfo, cJSON* pJsonChild);

#endif // COREMCA_H