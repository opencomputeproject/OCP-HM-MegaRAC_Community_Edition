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
#include <errno.h>
#include <fcntl.h>
#include <peci.h>
#include <string.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#include <linux/peci-ioctl.h>
#pragma GCC diagnostic pop

EPECIStatus peci_GetDIB_seq(uint8_t target, uint64_t* dib, int peci_fd);

/*-------------------------------------------------------------------------
 * This function unlocks the peci interface
 *------------------------------------------------------------------------*/
void peci_Unlock(int peci_fd)
{
    if (close(peci_fd) != 0)
    {
        syslog(LOG_ERR, "PECI device failed to unlock.\n");
    }
}

#define PECI_DEVICE "/dev/peci-0"
/*-------------------------------------------------------------------------
 * This function attempts to lock the peci interface with the specified
 * timeout and returns a file descriptor if successful.
 *------------------------------------------------------------------------*/
EPECIStatus peci_Lock(int* peci_fd, int timeout_ms)
{
    struct timespec sRequest;
    sRequest.tv_sec = 0;
    sRequest.tv_nsec = PECI_TIMEOUT_RESOLUTION_MS * 1000 * 1000;
    int timeout_count = 0;

    if (NULL == peci_fd)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Open the PECI driver with the specified timeout
    *peci_fd = open(PECI_DEVICE, O_RDWR | O_CLOEXEC);
    switch (timeout_ms)
    {
        case PECI_NO_WAIT:
            break;
        case PECI_WAIT_FOREVER:
            while (-1 == *peci_fd)
            {
                nanosleep(&sRequest, NULL);
                *peci_fd = open(PECI_DEVICE, O_RDWR | O_CLOEXEC);
            }
        default:
            while (-1 == *peci_fd && timeout_count < timeout_ms)
            {
                nanosleep(&sRequest, NULL);
                timeout_count += PECI_TIMEOUT_RESOLUTION_MS;
                *peci_fd = open(PECI_DEVICE, O_RDWR | O_CLOEXEC);
            }
    }
    if (-1 == *peci_fd)
    {
        syslog(LOG_ERR, " >>> PECI Device Busy <<< \n");
        return PECI_CC_DRIVER_ERR;
    }
    return PECI_CC_SUCCESS;
}

/*-------------------------------------------------------------------------
 * This function closes the peci interface
 *------------------------------------------------------------------------*/
static void peci_Close(int peci_fd)
{
    peci_Unlock(peci_fd);
}

/*-------------------------------------------------------------------------
 * This function opens the peci interface and returns a file descriptor
 *------------------------------------------------------------------------*/
static EPECIStatus peci_Open(int* peci_fd)
{
    if (NULL == peci_fd)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Lock the PECI driver with a default timeout
    return peci_Lock(peci_fd, PECI_TIMEOUT_MS);
}

/*-------------------------------------------------------------------------
 * This function issues peci commands to peci driver
 *------------------------------------------------------------------------*/
static EPECIStatus HW_peci_issue_cmd(unsigned int cmd, char* cmdPtr,
                                     int peci_fd)
{
    if (cmdPtr == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (ioctl(peci_fd, cmd, cmdPtr) != 0)
    {
        if (errno == ETIMEDOUT)
        {
            return PECI_CC_TIMEOUT;
        }
        return PECI_CC_DRIVER_ERR;
    }

    return PECI_CC_SUCCESS;
}

/*-------------------------------------------------------------------------
 * Find the specified PCI bus number value
 *------------------------------------------------------------------------*/
EPECIStatus FindBusNumber(uint8_t u8Bus, uint8_t u8Cpu, uint8_t* pu8BusValue)
{
    uint8_t u8CpuBus0[] = {
        PECI_PCI_BUS0_CPU0,
        PECI_PCI_BUS0_CPU1,
    };
    uint8_t u8Bus0 = 0;
    uint8_t u8Offset = 0;
    EPECIStatus ret;
    uint8_t u8Reg[4];
    uint8_t cc = 0;

    // First check for valid inputs
    // Check cpu and bus numbers, only support buses [5:0]
    if ((u8Bus > 5) || (u8Cpu >= (sizeof(u8CpuBus0) / sizeof(uint8_t))) ||
        (pu8BusValue == NULL))
    {
        return PECI_CC_INVALID_REQ;
    }

    // Get the Bus 0 value for the requested CPU
    u8Bus0 = u8CpuBus0[u8Cpu];

    // Next check that the bus numbers are valid
    // CPUBUSNO_VALID register - Above registers valid? - B(0) D5 F0 offset
    // D4h
    ret = peci_RdPCIConfig(u8Cpu, u8Bus0, PECI_PCI_CPUBUSNO_DEV,
                           PECI_PCI_CPUBUSNO_FUNC, PECI_PCI_CPUBUSNO_VALID,
                           u8Reg, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        return ret;
    }
    // BIOS will set bit 31 of CPUBUSNO_VALID when the bus numbers are valid
    if ((u8Reg[3] & 0x80) == 0)
    {
        return PECI_CC_HW_ERR;
    }

    // Bus numbers are valid so read the correct offset for the requested
    // bus CPUBUSNO register - CPU Internal Bus Numbers [3:0] - B(0) D5 F0
    // offset CCh CPUBUSNO_1 register - CPU Internal Bus Numbers [5:4] -
    // B(0) D5 F0 offset D0h
    u8Offset = u8Bus <= 3 ? PECI_PCI_CPUBUSNO : PECI_PCI_CPUBUSNO_1;
    ret = peci_RdPCIConfig(u8Cpu, u8Bus0, PECI_PCI_CPUBUSNO_DEV,
                           PECI_PCI_CPUBUSNO_FUNC, u8Offset, u8Reg, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        return ret;
    }

    // Now return the bus value for the requested bus
    *pu8BusValue = u8Reg[u8Bus % 4];

    // Unused bus numbers are set to zero which is only valid for bus 0
    // so, return an error for any other bus set to zero
    if (*pu8BusValue == 0 && u8Bus != 0)
    {
        return PECI_CC_CPU_NOT_PRESENT;
    }

    return PECI_CC_SUCCESS;
}

/*-------------------------------------------------------------------------
 * This function checks the CPU PECI interface
 *------------------------------------------------------------------------*/
EPECIStatus peci_Ping(uint8_t target)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_Ping_seq(target, peci_fd);

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential Ping with the provided
 * peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_Ping_seq(uint8_t target, int peci_fd)
{
    EPECIStatus ret;
    struct peci_ping_msg cmd;

    cmd.addr = target;
    ret = HW_peci_issue_cmd(PECI_IOC_PING, (char*)&cmd, peci_fd);

    return ret;
}

/*-------------------------------------------------------------------------
 * This function gets PECI device information
 *------------------------------------------------------------------------*/
EPECIStatus peci_GetDIB(uint8_t target, uint64_t* dib)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (dib == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_GetDIB_seq(target, dib, peci_fd);

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential GetDIB with the provided
 * peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_GetDIB_seq(uint8_t target, uint64_t* dib, int peci_fd)
{
    struct peci_get_dib_msg cmd;
    EPECIStatus ret;
    cmd.addr = target;

    if (dib == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    ret = HW_peci_issue_cmd(PECI_IOC_GET_DIB, (char*)&cmd, peci_fd);

    if (ret == PECI_CC_SUCCESS)
    {
        *dib = cmd.dib;
    }

    return ret;
}

/*-------------------------------------------------------------------------
 * This function get PECI Thermal temperature
 * Expressed in signed fixed point value of 1/64 degrees celsius
 *------------------------------------------------------------------------*/
EPECIStatus peci_GetTemp(uint8_t target, int16_t* temperature)
{
    int peci_fd = -1;
    struct peci_get_temp_msg cmd;

    if (temperature == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }

    cmd.addr = target;

    EPECIStatus ret =
        HW_peci_issue_cmd(PECI_IOC_GET_TEMP, (char*)&cmd, peci_fd);

    if (ret == PECI_CC_SUCCESS)
    {
        *temperature = cmd.temp_raw;
    }

    peci_Close(peci_fd);

    return ret;
}

/*-------------------------------------------------------------------------
 * This function provides read access to the package configuration
 * space within the processor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdPkgConfig(uint8_t target, uint8_t u8Index, uint16_t u16Value,
                             uint8_t u8ReadLen, uint8_t* pPkgConfig,
                             uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (pPkgConfig == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_RdPkgConfig_seq(target, u8Index, u16Value, u8ReadLen, pPkgConfig,
                               peci_fd, cc);

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential RdPkgConfig with the provided
 * peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdPkgConfig_seq(uint8_t target, uint8_t u8Index,
                                 uint16_t u16Value, uint8_t u8ReadLen,
                                 uint8_t* pPkgConfig, int peci_fd, uint8_t* cc)
{
    struct peci_rd_pkg_cfg_msg cmd;
    EPECIStatus ret;

    if (pPkgConfig == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the write length must be a byte, word, or dword
    if (u8ReadLen != 1 && u8ReadLen != 2 && u8ReadLen != 4)
    {
        return PECI_CC_INVALID_REQ;
    }

    // The PECI buffer must be large enough to hold the requested data
    if (sizeof(cmd.pkg_config) < u8ReadLen)
    {
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.index = u8Index;  // RdPkgConfig index
    cmd.param = u16Value; // Config parameter value
    cmd.rx_len = u8ReadLen;

    ret = HW_peci_issue_cmd(PECI_IOC_RD_PKG_CFG, (char*)&cmd, peci_fd);
    *cc = cmd.cc;
    if (ret == PECI_CC_SUCCESS)
    {
        memcpy(pPkgConfig, cmd.pkg_config, u8ReadLen);
    }

    return ret;
}

/*-------------------------------------------------------------------------
 * This function provides write access to the package configuration
 * space within the processor
 *------------------------------------------------------------------------*/
EPECIStatus peci_WrPkgConfig(uint8_t target, uint8_t u8Index, uint16_t u16Param,
                             uint32_t u32Value, uint8_t u8WriteLen, uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_WrPkgConfig_seq(target, u8Index, u16Param, u32Value, u8WriteLen,
                               peci_fd, cc);

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential WrPkgConfig with the provided
 * peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_WrPkgConfig_seq(uint8_t target, uint8_t u8Index,
                                 uint16_t u16Param, uint32_t u32Value,
                                 uint8_t u8WriteLen, int peci_fd, uint8_t* cc)
{
    struct peci_wr_pkg_cfg_msg cmd;
    EPECIStatus ret;

    if (cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the write length must be a byte, word, or dword
    if ((u8WriteLen != 1) && (u8WriteLen != 2) && (u8WriteLen != 4))
    {
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.index = u8Index;  // RdPkgConfig index
    cmd.param = u16Param; // parameter value
    cmd.tx_len = u8WriteLen;
    cmd.value = u32Value;

    ret = HW_peci_issue_cmd(PECI_IOC_WR_PKG_CFG, (char*)&cmd, peci_fd);
    *cc = cmd.cc;

    return ret;
}

/*-------------------------------------------------------------------------
 * This function provides read access to Model Specific Registers
 * defined in the processor doc.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdIAMSR(uint8_t target, uint8_t threadID, uint16_t MSRAddress,
                         uint64_t* u64MsrVal, uint8_t* cc)
{
    int peci_fd = -1;
    struct peci_rd_ia_msr_msg cmd;
    EPECIStatus ret;

    if (u64MsrVal == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }

    cmd.addr = target;
    cmd.thread_id = threadID; // request byte for thread ID
    cmd.address = MSRAddress; // MSR Address

    ret = HW_peci_issue_cmd(PECI_IOC_RD_IA_MSR, (char*)&cmd, peci_fd);
    *cc = cmd.cc;
    if (ret == PECI_CC_SUCCESS)
    {
        *u64MsrVal = cmd.value;
    }

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function provides read access to the PCI configuration space at
 * the requested PCI configuration address.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdPCIConfig(uint8_t target, uint8_t u8Bus, uint8_t u8Device,
                             uint8_t u8Fcn, uint16_t u16Reg, uint8_t* pPCIData,
                             uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (pPCIData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_RdPCIConfig_seq(target, u8Bus, u8Device, u8Fcn, u16Reg, pPCIData,
                               peci_fd, cc);

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential RdPCIConfig with the provided
 * peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdPCIConfig_seq(uint8_t target, uint8_t u8Bus,
                                 uint8_t u8Device, uint8_t u8Fcn,
                                 uint16_t u16Reg, uint8_t* pPCIData,
                                 int peci_fd, uint8_t* cc)
{
    struct peci_rd_pci_cfg_msg cmd;
    EPECIStatus ret;

    if (pPCIData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // The PECI buffer must be large enough to hold the PCI data
    if (sizeof(cmd.pci_config) < 4)
    {
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.bus = u8Bus;
    cmd.device = u8Device;
    cmd.function = u8Fcn;
    cmd.reg = u16Reg;

    ret = HW_peci_issue_cmd(PECI_IOC_RD_PCI_CFG, (char*)&cmd, peci_fd);
    *cc = cmd.cc;

    if (ret == PECI_CC_SUCCESS)
    {
        memcpy(pPCIData, cmd.pci_config, 4);
    }

    return ret;
}

/*-------------------------------------------------------------------------
 *  This function provides read access to the local PCI configuration space
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdPCIConfigLocal(uint8_t target, uint8_t u8Bus,
                                  uint8_t u8Device, uint8_t u8Fcn,
                                  uint16_t u16Reg, uint8_t u8ReadLen,
                                  uint8_t* pPCIReg, uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (pPCIReg == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_RdPCIConfigLocal_seq(target, u8Bus, u8Device, u8Fcn, u16Reg,
                                    u8ReadLen, pPCIReg, peci_fd, cc);

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential RdPCIConfigLocal with the provided
 * peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdPCIConfigLocal_seq(uint8_t target, uint8_t u8Bus,
                                      uint8_t u8Device, uint8_t u8Fcn,
                                      uint16_t u16Reg, uint8_t u8ReadLen,
                                      uint8_t* pPCIReg, int peci_fd,
                                      uint8_t* cc)
{
    struct peci_rd_pci_cfg_local_msg cmd;
    EPECIStatus ret;

    if (pPCIReg == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the read length must be a byte, word, or dword
    if (u8ReadLen != 1 && u8ReadLen != 2 && u8ReadLen != 4)
    {
        return PECI_CC_INVALID_REQ;
    }

    // The PECI buffer must be large enough to hold the requested data
    if (sizeof(cmd.pci_config) < u8ReadLen)
    {
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.bus = u8Bus;
    cmd.device = u8Device;
    cmd.function = u8Fcn;
    cmd.reg = u16Reg;
    cmd.rx_len = u8ReadLen;

    ret = HW_peci_issue_cmd(PECI_IOC_RD_PCI_CFG_LOCAL, (char*)&cmd, peci_fd);
    *cc = cmd.cc;

    if (ret == PECI_CC_SUCCESS)
    {
        memcpy(pPCIReg, cmd.pci_config, u8ReadLen);
    }

    return ret;
}

/*-------------------------------------------------------------------------
 *  This function provides write access to the local PCI configuration space
 *------------------------------------------------------------------------*/
EPECIStatus peci_WrPCIConfigLocal(uint8_t target, uint8_t u8Bus,
                                  uint8_t u8Device, uint8_t u8Fcn,
                                  uint16_t u16Reg, uint8_t DataLen,
                                  uint32_t DataVal, uint8_t* cc)
{
    int peci_fd = -1;
    struct peci_wr_pci_cfg_local_msg cmd;
    EPECIStatus ret;

    if (cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }

    // Per the PECI spec, the write length must be a byte, word, or dword
    if (DataLen != 1 && DataLen != 2 && DataLen != 4)
    {
        peci_Close(peci_fd);
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.bus = u8Bus;
    cmd.device = u8Device;
    cmd.function = u8Fcn;
    cmd.reg = u16Reg;
    cmd.tx_len = DataLen;
    cmd.value = DataVal;

    ret = HW_peci_issue_cmd(PECI_IOC_WR_PCI_CFG_LOCAL, (char*)&cmd, peci_fd);
    *cc = cmd.cc;

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This internal function is the common interface for RdEndPointConfig to PCI
 *------------------------------------------------------------------------*/
static EPECIStatus peci_RdEndPointConfigPciCommon(
    uint8_t target, uint8_t u8MsgType, uint8_t u8Seg, uint8_t u8Bus,
    uint8_t u8Device, uint8_t u8Fcn, uint16_t u16Reg, uint8_t u8ReadLen,
    uint8_t* pPCIData, int peci_fd, uint8_t* cc)
{
    struct peci_rd_end_pt_cfg_msg cmd;
    EPECIStatus ret;

    if (pPCIData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // The PECI buffer must be large enough to hold the requested data
    if (sizeof(cmd.data) < u8ReadLen)
    {
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.msg_type = u8MsgType;
    cmd.params.pci_cfg.seg = u8Seg;
    cmd.params.pci_cfg.bus = u8Bus;
    cmd.params.pci_cfg.device = u8Device;
    cmd.params.pci_cfg.function = u8Fcn;
    cmd.params.pci_cfg.reg = u16Reg;
    cmd.rx_len = u8ReadLen;

    ret = HW_peci_issue_cmd(PECI_IOC_RD_END_PT_CFG, (char*)&cmd, peci_fd);
    *cc = cmd.cc;

    if (ret == PECI_CC_SUCCESS)
    {
        memcpy(pPCIData, cmd.data, u8ReadLen);
    }
    else
    {
        ret = PECI_CC_DRIVER_ERR;
    }

    return ret;
}

/*-------------------------------------------------------------------------
 * This function provides read access to the PCI configuration space at
 * the requested PCI configuration address.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdEndPointConfigPci(uint8_t target, uint8_t u8Seg,
                                     uint8_t u8Bus, uint8_t u8Device,
                                     uint8_t u8Fcn, uint16_t u16Reg,
                                     uint8_t u8ReadLen, uint8_t* pPCIData,
                                     uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (pPCIData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret =
        peci_RdEndPointConfigPci_seq(target, u8Seg, u8Bus, u8Device, u8Fcn,
                                     u16Reg, u8ReadLen, pPCIData, peci_fd, cc);
    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential RdEndPointConfig to PCI with the provided
 * peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdEndPointConfigPci_seq(uint8_t target, uint8_t u8Seg,
                                         uint8_t u8Bus, uint8_t u8Device,
                                         uint8_t u8Fcn, uint16_t u16Reg,
                                         uint8_t u8ReadLen, uint8_t* pPCIData,
                                         int peci_fd, uint8_t* cc)
{
    if (pPCIData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the read length must be a byte, word, or dword
    if (u8ReadLen != 1 && u8ReadLen != 2 && u8ReadLen != 4)
    {
        return PECI_CC_INVALID_REQ;
    }

    return peci_RdEndPointConfigPciCommon(target, PECI_ENDPTCFG_TYPE_PCI, u8Seg,
                                          u8Bus, u8Device, u8Fcn, u16Reg,
                                          u8ReadLen, pPCIData, peci_fd, cc);
}

/*-------------------------------------------------------------------------
 * This function provides read access to the Local PCI configuration space at
 * the requested PCI configuration address.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdEndPointConfigPciLocal(uint8_t target, uint8_t u8Seg,
                                          uint8_t u8Bus, uint8_t u8Device,
                                          uint8_t u8Fcn, uint16_t u16Reg,
                                          uint8_t u8ReadLen, uint8_t* pPCIData,
                                          uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (pPCIData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_RdEndPointConfigPciLocal_seq(target, u8Seg, u8Bus, u8Device,
                                            u8Fcn, u16Reg, u8ReadLen, pPCIData,
                                            peci_fd, cc);
    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential RdEndPointConfig to PCI Local with the
 *provided peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdEndPointConfigPciLocal_seq(uint8_t target, uint8_t u8Seg,
                                              uint8_t u8Bus, uint8_t u8Device,
                                              uint8_t u8Fcn, uint16_t u16Reg,
                                              uint8_t u8ReadLen,
                                              uint8_t* pPCIData, int peci_fd,
                                              uint8_t* cc)
{
    if (pPCIData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the read length must be a byte, word, or dword
    if (u8ReadLen != 1 && u8ReadLen != 2 && u8ReadLen != 4)
    {
        return PECI_CC_INVALID_REQ;
    }

    return peci_RdEndPointConfigPciCommon(target, PECI_ENDPTCFG_TYPE_LOCAL_PCI,
                                          u8Seg, u8Bus, u8Device, u8Fcn, u16Reg,
                                          u8ReadLen, pPCIData, peci_fd, cc);
}

/*-------------------------------------------------------------------------
 * This function provides read access to PCI MMIO space at
 * the requested PCI configuration address.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdEndPointConfigMmio(uint8_t target, uint8_t u8Seg,
                                      uint8_t u8Bus, uint8_t u8Device,
                                      uint8_t u8Fcn, uint8_t u8Bar,
                                      uint8_t u8AddrType, uint64_t u64Offset,
                                      uint8_t u8ReadLen, uint8_t* pMmioData,
                                      uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (pMmioData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_RdEndPointConfigMmio_seq(target, u8Seg, u8Bus, u8Device, u8Fcn,
                                        u8Bar, u8AddrType, u64Offset, u8ReadLen,
                                        pMmioData, peci_fd, cc);
    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential RdEndPointConfig to PCI MMIO with the
 *provided peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_RdEndPointConfigMmio_seq(
    uint8_t target, uint8_t u8Seg, uint8_t u8Bus, uint8_t u8Device,
    uint8_t u8Fcn, uint8_t u8Bar, uint8_t u8AddrType, uint64_t u64Offset,
    uint8_t u8ReadLen, uint8_t* pMmioData, int peci_fd, uint8_t* cc)
{
    struct peci_rd_end_pt_cfg_msg cmd;
    EPECIStatus ret;

    if (pMmioData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the read length must be a byte, word, dword, or qword
    if (u8ReadLen != 1 && u8ReadLen != 2 && u8ReadLen != 4 && u8ReadLen != 8)
    {
        return PECI_CC_INVALID_REQ;
    }

    // The PECI buffer must be large enough to hold the requested data
    if (sizeof(cmd.data) < u8ReadLen)
    {
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.msg_type = PECI_ENDPTCFG_TYPE_MMIO;
    cmd.params.mmio.seg = u8Seg;
    cmd.params.mmio.bus = u8Bus;
    cmd.params.mmio.device = u8Device;
    cmd.params.mmio.function = u8Fcn;
    cmd.params.mmio.bar = u8Bar;
    cmd.params.mmio.addr_type = u8AddrType;
    cmd.params.mmio.offset = u64Offset;
    cmd.rx_len = u8ReadLen;

    ret = HW_peci_issue_cmd(PECI_IOC_RD_END_PT_CFG, (char*)&cmd, peci_fd);
    *cc = cmd.cc;

    if (ret == PECI_CC_SUCCESS)
    {
        memcpy(pMmioData, cmd.data, u8ReadLen);
    }
    else
    {
        ret = PECI_CC_DRIVER_ERR;
    }

    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential peci_WrEndPointConfig to PCI EndPoint with
 *the provided peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_WrEndPointConfig_seq(uint8_t target, uint8_t u8MsgType,
                                      uint8_t u8Seg, uint8_t u8Bus,
                                      uint8_t u8Device, uint8_t u8Fcn,
                                      uint16_t u16Reg, uint8_t DataLen,
                                      uint32_t DataVal, int peci_fd,
                                      uint8_t* cc)
{
    struct peci_wr_end_pt_cfg_msg cmd;
    EPECIStatus ret;

    if (cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the write length must be a byte, word, or dword
    if (DataLen != 1 && DataLen != 2 && DataLen != 4)
    {
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.msg_type = u8MsgType;
    cmd.params.pci_cfg.seg = u8Seg;
    cmd.params.pci_cfg.bus = u8Bus;
    cmd.params.pci_cfg.device = u8Device;
    cmd.params.pci_cfg.function = u8Fcn;
    cmd.params.pci_cfg.reg = u16Reg;
    cmd.tx_len = DataLen;
    cmd.value = DataVal;

    ret = HW_peci_issue_cmd(PECI_IOC_WR_END_PT_CFG, (char*)&cmd, peci_fd);
    *cc = cmd.cc;

    return ret;
}

/*-------------------------------------------------------------------------
 *  This function provides write access to the EP local PCI configuration space
 *------------------------------------------------------------------------*/
EPECIStatus peci_WrEndPointPCIConfigLocal(uint8_t target, uint8_t u8Seg,
                                          uint8_t u8Bus, uint8_t u8Device,
                                          uint8_t u8Fcn, uint16_t u16Reg,
                                          uint8_t DataLen, uint32_t DataVal,
                                          uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }

    ret = peci_WrEndPointConfig_seq(target, PECI_ENDPTCFG_TYPE_LOCAL_PCI, u8Seg,
                                    u8Bus, u8Device, u8Fcn, u16Reg, DataLen,
                                    DataVal, peci_fd, cc);
    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 *  This function provides write access to the EP local PCI configuration space
 *------------------------------------------------------------------------*/
EPECIStatus peci_WrEndPointPCIConfig(uint8_t target, uint8_t u8Seg,
                                     uint8_t u8Bus, uint8_t u8Device,
                                     uint8_t u8Fcn, uint16_t u16Reg,
                                     uint8_t DataLen, uint32_t DataVal,
                                     uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_WrEndPointConfig_seq(target, PECI_ENDPTCFG_TYPE_PCI, u8Seg,
                                    u8Bus, u8Device, u8Fcn, u16Reg, DataLen,
                                    DataVal, peci_fd, cc);
    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function provides write access to PCI MMIO space at
 * the requested PCI configuration address.
 *------------------------------------------------------------------------*/
EPECIStatus peci_WrEndPointConfigMmio(uint8_t target, uint8_t u8Seg,
                                      uint8_t u8Bus, uint8_t u8Device,
                                      uint8_t u8Fcn, uint8_t u8Bar,
                                      uint8_t u8AddrType, uint64_t u64Offset,
                                      uint8_t u8DataLen, uint64_t u64DataVal,
                                      uint8_t* cc)
{
    int peci_fd = -1;
    EPECIStatus ret;

    if (cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }
    ret = peci_WrEndPointConfigMmio_seq(target, u8Seg, u8Bus, u8Device, u8Fcn,
                                        u8Bar, u8AddrType, u64Offset, u8DataLen,
                                        u64DataVal, peci_fd, cc);
    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function allows sequential WrEndPointConfig to PCI MMIO with the
 * provided peci file descriptor.
 *------------------------------------------------------------------------*/
EPECIStatus peci_WrEndPointConfigMmio_seq(
    uint8_t target, uint8_t u8Seg, uint8_t u8Bus, uint8_t u8Device,
    uint8_t u8Fcn, uint8_t u8Bar, uint8_t u8AddrType, uint64_t u64Offset,
    uint8_t u8DataLen, uint64_t u64DataVal, int peci_fd, uint8_t* cc)
{
    struct peci_wr_end_pt_cfg_msg cmd;
    EPECIStatus ret;

    if (cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the read length must be a byte, word, dword, or qword
    if (u8DataLen != 1 && u8DataLen != 2 && u8DataLen != 4 && u8DataLen != 8)
    {
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.msg_type = PECI_ENDPTCFG_TYPE_MMIO;
    cmd.params.mmio.seg = u8Seg;
    cmd.params.mmio.bus = u8Bus;
    cmd.params.mmio.device = u8Device;
    cmd.params.mmio.function = u8Fcn;
    cmd.params.mmio.bar = u8Bar;
    cmd.params.mmio.addr_type = u8AddrType;
    cmd.params.mmio.offset = u64Offset;
    cmd.tx_len = u8DataLen;
    cmd.value = u64DataVal;

    ret = HW_peci_issue_cmd(PECI_IOC_WR_END_PT_CFG, (char*)&cmd, peci_fd);
    *cc = cmd.cc;

    return ret;
}

/*-------------------------------------------------------------------------
 *  This function provides crashdump discovery data over PECI
 *------------------------------------------------------------------------*/
EPECIStatus peci_CrashDump_Discovery(uint8_t target, uint8_t subopcode,
                                     uint8_t param0, uint16_t param1,
                                     uint8_t param2, uint8_t u8ReadLen,
                                     uint8_t* pData, uint8_t* cc)
{
    int peci_fd = -1;
    struct peci_crashdump_disc_msg cmd;
    EPECIStatus ret;

    if (pData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the read length must be a byte, word, or qword
    if (u8ReadLen != 1 && u8ReadLen != 2 && u8ReadLen != 8)
    {
        return PECI_CC_INVALID_REQ;
    }

    // The PECI buffer must be large enough to hold the requested data
    if (sizeof(cmd.data) < u8ReadLen)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }

    cmd.addr = target;
    cmd.subopcode = subopcode;
    cmd.param0 = param0;
    cmd.param1 = param1;
    cmd.param2 = param2;
    cmd.rx_len = u8ReadLen;

    ret = HW_peci_issue_cmd(PECI_IOC_CRASHDUMP_DISC, (char*)&cmd, peci_fd);
    *cc = cmd.cc;
    if (ret == PECI_CC_SUCCESS)
    {
        memcpy(pData, cmd.data, u8ReadLen);
    }
    else
    {
        ret = PECI_CC_DRIVER_ERR;
    }

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 *  This function provides crashdump GetFrame data over PECI
 *------------------------------------------------------------------------*/
EPECIStatus peci_CrashDump_GetFrame(uint8_t target, uint16_t param0,
                                    uint16_t param1, uint16_t param2,
                                    uint8_t u8ReadLen, uint8_t* pData,
                                    uint8_t* cc)
{
    int peci_fd = -1;
    struct peci_crashdump_get_frame_msg cmd;
    EPECIStatus ret;

    if (pData == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    // Per the PECI spec, the read length must be a qword or dqword
    if (u8ReadLen != 8 && u8ReadLen != 16)
    {
        return PECI_CC_INVALID_REQ;
    }

    // The PECI buffer must be large enough to hold the requested data
    if (sizeof(cmd.data) < u8ReadLen)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }

    cmd.addr = target;
    cmd.param0 = param0;
    cmd.param1 = param1;
    cmd.param2 = param2;
    cmd.rx_len = u8ReadLen;

    ret = HW_peci_issue_cmd(PECI_IOC_CRASHDUMP_GET_FRAME, (char*)&cmd, peci_fd);
    *cc = cmd.cc;
    if (ret == PECI_CC_SUCCESS)
    {
        memcpy(pData, cmd.data, u8ReadLen);
    }
    else
    {
        ret = PECI_CC_DRIVER_ERR;
    }

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 *  This function provides raw PECI command access
 *------------------------------------------------------------------------*/
EPECIStatus peci_raw(uint8_t target, uint8_t u8ReadLen, const uint8_t* pRawCmd,
                     const uint32_t cmdSize, uint8_t* pRawResp,
                     uint32_t respSize)
{
    int peci_fd = -1;
    struct peci_xfer_msg cmd;
    uint8_t u8TxBuf[PECI_BUFFER_SIZE];
    uint8_t u8RxBuf[PECI_BUFFER_SIZE];
    EPECIStatus ret;

    if (u8ReadLen && pRawResp == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Open(&peci_fd) != PECI_CC_SUCCESS)
    {
        return PECI_CC_DRIVER_ERR;
    }

    // Check for valid buffer sizes
    if (cmdSize > PECI_BUFFER_SIZE || respSize < u8ReadLen ||
        u8ReadLen >
            (PECI_BUFFER_SIZE - 1)) // response buffer is data + 1 status byte
    {
        peci_Close(peci_fd);
        return PECI_CC_INVALID_REQ;
    }

    cmd.addr = target;
    cmd.tx_len = (uint8_t)cmdSize;
    cmd.rx_len = u8ReadLen;

    memcpy(u8TxBuf, pRawCmd, cmdSize);

    cmd.tx_buf = u8TxBuf;
    cmd.rx_buf = u8RxBuf;
    ret = HW_peci_issue_cmd(PECI_IOC_XFER, (char*)&cmd, peci_fd);

    if (ret == PECI_CC_SUCCESS || ret == PECI_CC_TIMEOUT)
    {
        memcpy(pRawResp, u8RxBuf, u8ReadLen);
    }

    peci_Close(peci_fd);
    return ret;
}

/*-------------------------------------------------------------------------
 * This function returns the CPUID (Model and stepping) for the given PECI
 * client address
 *------------------------------------------------------------------------*/
EPECIStatus peci_GetCPUID(const uint8_t clientAddr, CPUModel* cpuModel,
                          uint8_t* stepping, uint8_t* cc)
{
    EPECIStatus ret = PECI_CC_SUCCESS;
    uint32_t cpuid = 0;

    if (cpuModel == NULL || stepping == NULL || cc == NULL)
    {
        return PECI_CC_INVALID_REQ;
    }

    if (peci_Ping(clientAddr) != PECI_CC_SUCCESS)
    {
        return PECI_CC_CPU_NOT_PRESENT;
    }

    ret =
        peci_RdPkgConfig(clientAddr, PECI_MBX_INDEX_CPU_ID, PECI_PKG_ID_CPU_ID,
                         sizeof(uint32_t), (uint8_t*)&cpuid, cc);

    // Separate out the model and stepping (bits 3:0) from the CPUID
    *cpuModel = cpuid & 0xFFFFFFF0;
    *stepping = (uint8_t)(cpuid & 0x0000000F);
    return ret;
}
