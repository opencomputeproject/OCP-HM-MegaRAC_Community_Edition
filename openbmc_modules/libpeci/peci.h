/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#include <stdbool.h>

// PECI Client Address List
#define MIN_CLIENT_ADDR 0x30
#define MAX_CLIENT_ADDR 0x37
#define MAX_CPUS (MAX_CLIENT_ADDR - MIN_CLIENT_ADDR + 1)

// PECI completion codes from peci-ioctl.h
#define PECI_DEV_CC_SUCCESS 0x40
#define PECI_DEV_CC_FATAL_MCA_DETECTED 0x94

typedef enum
{
    skx = 0x00050650,
    icx = 0x000606A0,
} CPUModel;

// PECI Status Codes
typedef enum
{
    PECI_CC_SUCCESS = 0,
    PECI_CC_INVALID_REQ,
    PECI_CC_HW_ERR,
    PECI_CC_DRIVER_ERR,
    PECI_CC_CPU_NOT_PRESENT,
    PECI_CC_MEM_ERR,
    PECI_CC_TIMEOUT,
} EPECIStatus;

// PECI Timeout Options
typedef enum
{
    PECI_WAIT_FOREVER = -1,
    PECI_NO_WAIT = 0,
} EPECITimeout;

#define PECI_TIMEOUT_RESOLUTION_MS 10 // 10 ms
#define PECI_TIMEOUT_MS 100           // 100 ms

// VCU Index and Sequence Paramaters
#define VCU_SET_PARAM 0x0001
#define VCU_READ 0x0002
#define VCU_OPEN_SEQ 0x0003
#define VCU_CLOSE_SEQ 0x0004
#define VCU_ABORT_SEQ 0x0005
#define VCU_VERSION 0x0009

typedef enum
{
    VCU_READ_LOCAL_CSR_SEQ = 0x2,
    VCU_READ_LOCAL_MMIO_SEQ = 0x6,
    VCU_EN_SECURE_DATA_SEQ = 0x14,
    VCU_CORE_MCA_SEQ = 0x10000,
    VCU_UNCORE_MCA_SEQ = 0x10000,
    VCU_IOT_BRKPT_SEQ = 0x10010,
    VCU_MBP_CONFIG_SEQ = 0x10026,
    VCU_PWR_MGT_SEQ = 0x1002a,
    VCU_CRASHDUMP_SEQ = 0x10038,
    VCU_ARRAY_DUMP_SEQ = 0x20000,
    VCU_SCAN_DUMP_SEQ = 0x20008,
    VCU_TOR_DUMP_SEQ = 0x30002,
    VCU_SQ_DUMP_SEQ = 0x30004,
    VCU_UNCORE_CRASHDUMP_SEQ = 0x30006,
} EPECISequence;

#define MBX_INDEX_VCU 128 // VCU Index

typedef enum
{
    MMIO_DWORD_OFFSET = 0x05,
    MMIO_QWORD_OFFSET = 0x06,
} EEndPtMmioAddrType;

// Find the specified PCI bus number value
EPECIStatus FindBusNumber(uint8_t u8Bus, uint8_t u8Cpu, uint8_t* pu8BusValue);

// Gets the temperature from the target
// Expressed in signed fixed point value of 1/64 degrees celsius
EPECIStatus peci_GetTemp(uint8_t target, int16_t* temperature);

// Provides read access to the package configuration space within the processor
EPECIStatus peci_RdPkgConfig(uint8_t target, uint8_t u8Index, uint16_t u16Value,
                             uint8_t u8ReadLen, uint8_t* pPkgConfig,
                             uint8_t* cc);

// Allows sequential RdPkgConfig with the provided peci file descriptor
EPECIStatus peci_RdPkgConfig_seq(uint8_t target, uint8_t u8Index,
                                 uint16_t u16Value, uint8_t u8ReadLen,
                                 uint8_t* pPkgConfig, int peci_fd, uint8_t* cc);

// Provides write access to the package configuration space within the processor
EPECIStatus peci_WrPkgConfig(uint8_t target, uint8_t u8Index, uint16_t u16Param,
                             uint32_t u32Value, uint8_t u8WriteLen,
                             uint8_t* cc);

// Allows sequential WrPkgConfig with the provided peci file descriptor
EPECIStatus peci_WrPkgConfig_seq(uint8_t target, uint8_t u8Index,
                                 uint16_t u16Param, uint32_t u32Value,
                                 uint8_t u8WriteLen, int peci_fd, uint8_t* cc);

// Provides read access to Model Specific Registers
EPECIStatus peci_RdIAMSR(uint8_t target, uint8_t threadID, uint16_t MSRAddress,
                         uint64_t* u64MsrVal, uint8_t* cc);

// Provides read access to PCI Configuration space
EPECIStatus peci_RdPCIConfig(uint8_t target, uint8_t u8Bus, uint8_t u8Device,
                             uint8_t u8Fcn, uint16_t u16Reg, uint8_t* pPCIReg,
                             uint8_t* cc);

// Allows sequential RdPCIConfig with the provided peci file descriptor
EPECIStatus peci_RdPCIConfig_seq(uint8_t target, uint8_t u8Bus,
                                 uint8_t u8Device, uint8_t u8Fcn,
                                 uint16_t u16Reg, uint8_t* pPCIData,
                                 int peci_fd, uint8_t* cc);

// Provides read access to the local PCI Configuration space
EPECIStatus peci_RdPCIConfigLocal(uint8_t target, uint8_t u8Bus,
                                  uint8_t u8Device, uint8_t u8Fcn,
                                  uint16_t u16Reg, uint8_t u8ReadLen,
                                  uint8_t* pPCIReg, uint8_t* cc);

// Allows sequential RdPCIConfigLocal with the provided peci file descriptor
EPECIStatus peci_RdPCIConfigLocal_seq(uint8_t target, uint8_t u8Bus,
                                      uint8_t u8Device, uint8_t u8Fcn,
                                      uint16_t u16Reg, uint8_t u8ReadLen,
                                      uint8_t* pPCIReg, int peci_fd,
                                      uint8_t* cc);

// Provides write access to the local PCI Configuration space
EPECIStatus peci_WrPCIConfigLocal(uint8_t target, uint8_t u8Bus,
                                  uint8_t u8Device, uint8_t u8Fcn,
                                  uint16_t u16Reg, uint8_t DataLen,
                                  uint32_t DataVal, uint8_t* cc);

// Provides read access to PCI configuration space
EPECIStatus peci_RdEndPointConfigPci(uint8_t target, uint8_t u8Seg,
                                     uint8_t u8Bus, uint8_t u8Device,
                                     uint8_t u8Fcn, uint16_t u16Reg,
                                     uint8_t u8ReadLen, uint8_t* pPCIData,
                                     uint8_t* cc);

// Allows sequential RdEndPointConfig to PCI Configuration space
EPECIStatus peci_RdEndPointConfigPci_seq(uint8_t target, uint8_t u8Seg,
                                         uint8_t u8Bus, uint8_t u8Device,
                                         uint8_t u8Fcn, uint16_t u16Reg,
                                         uint8_t u8ReadLen, uint8_t* pPCIData,
                                         int peci_fd, uint8_t* cc);

// Provides read access to the local PCI configuration space
EPECIStatus peci_RdEndPointConfigPciLocal(uint8_t target, uint8_t u8Seg,
                                          uint8_t u8Bus, uint8_t u8Device,
                                          uint8_t u8Fcn, uint16_t u16Reg,
                                          uint8_t u8ReadLen, uint8_t* pPCIData,
                                          uint8_t* cc);

// Allows sequential RdEndPointConfig to the local PCI Configuration space
EPECIStatus peci_RdEndPointConfigPciLocal_seq(uint8_t target, uint8_t u8Seg,
                                              uint8_t u8Bus, uint8_t u8Device,
                                              uint8_t u8Fcn, uint16_t u16Reg,
                                              uint8_t u8ReadLen,
                                              uint8_t* pPCIData, int peci_fd,
                                              uint8_t* cc);

// Provides read access to PCI MMIO space
EPECIStatus peci_RdEndPointConfigMmio(uint8_t target, uint8_t u8Seg,
                                      uint8_t u8Bus, uint8_t u8Device,
                                      uint8_t u8Fcn, uint8_t u8Bar,
                                      uint8_t u8AddrType, uint64_t u64Offset,
                                      uint8_t u8ReadLen, uint8_t* pMmioData,
                                      uint8_t* cc);

// Allows sequential RdEndPointConfig to PCI MMIO space
EPECIStatus peci_RdEndPointConfigMmio_seq(
    uint8_t target, uint8_t u8Seg, uint8_t u8Bus, uint8_t u8Device,
    uint8_t u8Fcn, uint8_t u8Bar, uint8_t u8AddrType, uint64_t u64Offset,
    uint8_t u8ReadLen, uint8_t* pMmioData, int peci_fd, uint8_t* cc);

// Provides write access to the EP local PCI Configuration space
EPECIStatus peci_WrEndPointPCIConfigLocal(uint8_t target, uint8_t u8Seg,
                                          uint8_t u8Bus, uint8_t u8Device,
                                          uint8_t u8Fcn, uint16_t u16Reg,
                                          uint8_t DataLen, uint32_t DataVal,
                                          uint8_t* cc);

// Provides write access to the EP PCI Configuration space
EPECIStatus peci_WrEndPointPCIConfig(uint8_t target, uint8_t u8Seg,
                                     uint8_t u8Bus, uint8_t u8Device,
                                     uint8_t u8Fcn, uint16_t u16Reg,
                                     uint8_t DataLen, uint32_t DataVal,
                                     uint8_t* cc);

// Allows sequential write access to the EP PCI Configuration space
EPECIStatus peci_WrEndPointConfig_seq(uint8_t target, uint8_t u8MsgType,
                                      uint8_t u8Seg, uint8_t u8Bus,
                                      uint8_t u8Device, uint8_t u8Fcn,
                                      uint16_t u16Reg, uint8_t DataLen,
                                      uint32_t DataVal, int peci_fd,
                                      uint8_t* cc);

// Provides write access to the EP PCI MMIO space
EPECIStatus peci_WrEndPointConfigMmio(uint8_t target, uint8_t u8Seg,
                                      uint8_t u8Bus, uint8_t u8Device,
                                      uint8_t u8Fcn, uint8_t u8Bar,
                                      uint8_t u8AddrType, uint64_t u64Offset,
                                      uint8_t u8DataLen, uint64_t u64DataVal,
                                      uint8_t* cc);

// Allows sequential write access to the EP PCI MMIO space
EPECIStatus peci_WrEndPointConfigMmio_seq(
    uint8_t target, uint8_t u8Seg, uint8_t u8Bus, uint8_t u8Device,
    uint8_t u8Fcn, uint8_t u8Bar, uint8_t u8AddrType, uint64_t u64Offset,
    uint8_t u8DataLen, uint64_t u64DataVal, int peci_fd, uint8_t* cc);

// Provides access to the Crashdump Discovery API
EPECIStatus peci_CrashDump_Discovery(uint8_t target, uint8_t subopcode,
                                     uint8_t param0, uint16_t param1,
                                     uint8_t param2, uint8_t u8ReadLen,
                                     uint8_t* pData, uint8_t* cc);

// Provides access to the Crashdump GetFrame API
EPECIStatus peci_CrashDump_GetFrame(uint8_t target, uint16_t param0,
                                    uint16_t param1, uint16_t param2,
                                    uint8_t u8ReadLen, uint8_t* pData,
                                    uint8_t* cc);

// Provides raw PECI command access
EPECIStatus peci_raw(uint8_t target, uint8_t u8ReadLen, const uint8_t* pRawCmd,
                     const uint32_t cmdSize, uint8_t* pRawResp,
                     uint32_t respSize);

EPECIStatus peci_Lock(int* peci_fd, int timeout_ms);
void peci_Unlock(int peci_fd);
EPECIStatus peci_Ping(uint8_t target);
EPECIStatus peci_Ping_seq(uint8_t target, int peci_fd);
EPECIStatus peci_GetCPUID(const uint8_t clientAddr, CPUModel* cpuModel,
                          uint8_t* stepping, uint8_t* cc);

#ifdef __cplusplus
}
#endif
