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

#ifndef UNCORE_H
#define UNCORE_H

#include "crashdump.h"

/******************************************************************************
 *
 *   Common Defines
 *
 ******************************************************************************/
#define US_JSON_STRING_LEN 64

enum US_REG_SIZE
{
    US_REG_BYTE = 1,
    US_REG_WORD = 2,
    US_REG_DWORD = 4,
    US_REG_QWORD = 8
};

enum US_MMIO_SIZE
{
    US_MMIO_BYTE = 0,
    US_MMIO_WORD = 1,
    US_MMIO_DWORD = 2,
    US_MMIO_QWORD = 3
};

enum US_PCI
{
    US_PCI_REG_NAME,
    US_PCI_BUS,
    US_PCI_DEVICE,
    US_PCI_FUNCTION,
    US_PCI_OFFSET,
    US_PCI_SIZE
};

enum US_MMIO
{
    US_MMIO_REG_NAME,
    US_MMIO_BAR_ID,
    US_MMIO_BUS,
    US_MMIO_DEVICE,
    US_MMIO_FUNCTION,
    US_MMIO_OFFSET,
    US_MMIO_ADDRTYPE,
    US_MMIO_SIZE
};

enum US_RDIAMSR
{
    US_RDIAMSR_REG_NAME,
    US_RDIAMSR_ADDR,
    US_RDIAMSR_THREAD_ID,
    US_RDIAMSR_SIZE
};

#define US_REG_NAME_LEN 64
#define US_NUM_MCA_DWORDS 10
#define US_NUM_MCA_QWORDS (US_NUM_MCA_DWORDS / 2)

#define US_MCA_NAME_LEN 8
#define US_UNCORE_CRASH_DW_NAME "uncore_crashdump_dw%ld"

#define US_FAILED "N/A"
#define UNCORE_MCA_UA "UA:0x%x"
#define UNCORE_UA_DF "UA:0x%x,DF:0x%x"
#define UNCORE_FIXED_DATA_CC_RC "0x0,CC:0x%x,RC:0x%x"
#define UNCORE_DATA_CC_RC "0x%" PRIx64 ",CC:0x%x,RC:0x%x"

/******************************************************************************
 *
 *   ICX1 Defines
 *
 ******************************************************************************/
#define US_PCI_SEG 0
#define US_MMIO_SEG 0

#define MMIO_ROOT_BUS 0xD
#define MMIO_ROOT_DEV 0x0
#define MMIO_ROOT_FUNC 0x02
#define MMIO_ROOT_REG 0xCC

#define US_NA "N/A"
#define US_UA "UA:0x%x"
#define US_DF "DF:0x%x"
#define US_UINT64_FMT "0x%" PRIx64 ""
#define SIZE_FAILURE 7

/******************************************************************************
 *
 *   Input File Defines
 *
 ******************************************************************************/
#define FILE_PCI_KEY "_input_file_pci"
#define FILE_PCI_ERR "Error parsing pci section"
#define FILE_MMIO_KEY "_input_file_mmio"
#define FILE_MMIO_ERR "Error parsing mmio section"
#define FILE_RDIAMSR_KEY "_input_file_rdiamsr"
#define FILE_RDIAMSR_ERR "Error parsing rdiamsr section"

/******************************************************************************
 *
 *   Structures
 *
 ******************************************************************************/
typedef union
{
    uint64_t u64;
    uint32_t u32[2];
} UUncoreStatusRegValue;

typedef struct
{
    UUncoreStatusRegValue uValue;
    uint8_t cc;
    int ret;
    bool bInvalid;
} SUncoreStatusRegRawData;

typedef union
{
    uint64_t u64Raw[US_NUM_MCA_QWORDS];
    uint32_t u32Raw[US_NUM_MCA_DWORDS];
    struct
    {
        uint64_t ctl;
        uint64_t status;
        uint64_t addr;
        uint64_t misc;
        uint64_t misc2;
    } regs;
} UUncoreStatusMcaRegs;

typedef struct
{
    UUncoreStatusMcaRegs uRegData;
    uint8_t cc;
    int ret;
    bool bInvalid;
} SUncoreStatusMcaRawData;

typedef struct
{
    char regName[US_REG_NAME_LEN];
    uint8_t u8IioNum;
} SUncoreStatusRegIio;

typedef struct
{
    char* regName;
    uint8_t u8Bus;
    uint8_t u8Dev;
    uint8_t u8Func;
    uint16_t u16Reg;
    uint8_t u8Size;
    bool bTelemetry;
} SUncoreStatusRegPciIcx;

typedef struct
{
    char* regName;
    uint8_t u8Bar;
    uint8_t u8Bus;
    uint8_t u8Dev;
    uint8_t u8Func;
    uint64_t u64Offset;
    uint8_t u8AddrType;
    uint8_t u8Size;
    bool bTelemetry;
} SUncoreStatusRegPciMmioICXSPR;

typedef struct
{
    char* regName;
    uint16_t addr;
    uint8_t threadID;
    uint8_t u8Size;
    bool bTelemetry;
} SUncoreStatusMsrRegICXSPR;

static const char uncoreStatusMcaRegNames[][US_MCA_NAME_LEN] = {
    "ctl", "status", "addr", "misc", "ctl2"};

typedef int (*UncoreStatusRead)(CPUInfo* cpuInfo, cJSON* pJsonChild);

typedef struct
{
    Model cpuModel;
    int (*logUncoreStatusVx)(CPUInfo* cpuInfo, cJSON* pJsonChild);
} SUncoreStatusLogVx;

static const SUncoreStatusRegIio sUncoreStatusIio[] = {
    {"iio_cstack_mc_%s", 0},  {"iio_pstack0_mc_%s", 1},
    {"iio_pstack1_mc_%s", 2}, {"iio_pstack2_mc_%s", 3},
    {"iio_pstack3_mc_%s", 4},
};

int logUncoreStatus(CPUInfo* cpuInfo, cJSON* pJsonChild);

#endif // UNCORE_H