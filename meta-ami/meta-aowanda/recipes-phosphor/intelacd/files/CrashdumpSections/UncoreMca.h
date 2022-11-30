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

#ifndef UNCOREMCA_H
#define UNCOREMCA_H

#include "crashdump.h"

/******************************************************************************
 *
 *   Common Defines
 *
 ******************************************************************************/
#define UNCORE_MCA_NAME_LEN 16

#define UNCORE_MCA_JSON_STRING_LEN 64
#define UNCORE_MCA_JSON_SECTION_NAME "uncore"

#define UNCORE_MCA_JSON_CBO_NAME "CBO%d"
#define UNCORE_MCA_CBO_REG_NAME "cbo%ld_%s"

#define UNCORE_MCA_FAILED "N/A"
#define UNCORE_MCA_UA "UA:0x%x"
#define UNCORE_MCA_DF "DF:0x%x"
#define UNCORE_MCA_UA_DF "UA:0x%x,DF:0x%x"
#define UNCORE_MCA_FIXED_DATA_CC_RC "0x0,CC:0x%x,RC:0x%x"
#define UNCORE_MCA_DATA_CC_RC "0x%llx,CC:0x%x,RC:0x%x"

#define UCM_NUM_MCA_DWORDS 10
#define UCM_NUM_MCA_QWORDS (UCM_NUM_MCA_DWORDS / 2)

#define UNCORE_MCA_REG_NAME "mc%d_%s"
#define UNCORE_MCA_JSON_MCA_NAME "MC%d"

#define FILE_MCA_UC_KEY "_input_file_mca_uncore"
#define FILE_MCA_UC_ERR "Error parsing MCA uncore section"

/******************************************************************************
 *
 *   ICX/SPR Defines
 *
 ******************************************************************************/
enum MCA_UC_CBO
{
    MCA_UC_CBO_REG_NAME,
    MCA_UC_CBO_ADDR,
};

enum MCA_UC
{
    MCA_UC_BANK_NAME,
    MCA_UC_REG_NAME,
    MCA_UC_ADDR,
    MCA_UC_ID,
};

/******************************************************************************
 *
 *   ICX/SPR Structures
 *
 ******************************************************************************/
typedef struct
{
    char* bankName;
    char* regName;
    uint16_t addr;
    uint8_t instance_id;
} SUncoreMcaReg;

typedef struct
{
    char* regName;
    uint16_t addr;
    uint64_t data;
    bool valid;
    uint8_t cc;
} SUncoreMcaCboReg;

typedef struct
{
    Model cpuModel;
    int (*logUncoreMcaVx)(CPUInfo* cpuInfo, cJSON* pJsonChild);
} SUncoreMcaLogVx;

int logUncoreMca(CPUInfo* cpuInfo, cJSON* pJsonChild);

#endif // UNCOREMCA_H