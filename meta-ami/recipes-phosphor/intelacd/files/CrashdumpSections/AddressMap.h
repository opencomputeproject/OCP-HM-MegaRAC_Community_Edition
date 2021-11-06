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

#ifndef ADDRESSMAP_H
#define ADDRESSMAP_H

#include "crashdump.h"

#define AM_REG_NAME_LEN 34

#define AM_JSON_STRING_LEN 64

#define AM_NA "N/A"
#define AM_UA "UA:0x%x"
#define AM_DF "DF:0x%x"
#define AM_UA_DF "UA:0x%x,DF:0x%x"
#define AM_DATA_CC_RC "0x%" PRIx64 ",CC:0x%x,RC:0x%x"
#define AM_FIXED_DATA_CC_RC "0x0,CC:0x%x,RC:0x%x"
#define AM_UINT64_FMT "0x%" PRIx64 ""
#define SIZE_FAILURE 7
#define AM_PCI_SEG 0
#define FILE_ADDRESS_MAP_KEY "_input_file_address_map"
#define FILE_ADDRESS_MAP_ERR "Error parsing address map section"

/******************************************************************************
 *
 *   Structures
 *
 ******************************************************************************/
enum AM_REG_SIZE
{
    AM_REG_BYTE = 1,
    AM_REG_WORD = 2,
    AM_REG_DWORD = 4,
    AM_REG_QWORD = 8
};

enum ADDRESS_MAP_HEADER
{
    ADDRESS_MAP_REG_NAME,
    ADDRESS_MAP_BUS,
    ADDRESS_MAP_DEVICE,
    ADDRESS_MAP_FUNCTION,
    ADDRESS_MAP_OFFSET,
    ADDRESS_MAP_SIZE
};

typedef union
{
    uint64_t u64;
    uint32_t u32[2];
} UAddrMapRegValue;

typedef struct
{
    UAddrMapRegValue uValue;
    uint8_t cc;
    bool bInvalid;
} SAddressMapRegRawData;

typedef struct
{
    char* regName;
    uint8_t u8Bus;
    uint8_t u8Dev;
    uint8_t u8Func;
    uint16_t u16Reg;
    uint8_t u8Size;
} SAddrMapEntry;

typedef struct
{
    Model cpuModel;
    int (*logAddrMapVx)(CPUInfo* cpuInfo, cJSON* pJsonChild);
} SAddrMapVx;

int logAddressMap(CPUInfo* cpuInfo, cJSON* pJsonChild);

#endif // ADDRESSMAP_H