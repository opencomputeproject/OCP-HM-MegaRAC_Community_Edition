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

#include "Uncore.h"

#include "utils.h"

/******************************************************************************
 *
 *   uncoreStatusJsonICXSPR
 *
 *   This function formats the Uncore Status PCI registers into a JSON object
 *
 ******************************************************************************/
static void uncoreStatusJsonICXSPR(const char* regName,
                                   SUncoreStatusRegRawData* sRegData,
                                   cJSON* pJsonChild, uint8_t cc, int ret)
{
    char jsonItemString[US_JSON_STRING_LEN];

    if (sRegData->bInvalid)
    {
        cd_snprintf_s(jsonItemString, US_JSON_STRING_LEN,
                      UNCORE_FIXED_DATA_CC_RC, cc, ret);
        cJSON_AddStringToObject(pJsonChild, regName, jsonItemString);
        return;
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, US_JSON_STRING_LEN, UNCORE_DATA_CC_RC,
                      sRegData->uValue.u64, cc, ret);
        cJSON_AddStringToObject(pJsonChild, regName, jsonItemString);
        return;
    }
    else
    {
        cd_snprintf_s(jsonItemString, US_JSON_STRING_LEN, US_UINT64_FMT,
                      sRegData->uValue.u64, cc);
    }

    cJSON_AddStringToObject(pJsonChild, regName, jsonItemString);
}

/******************************************************************************
 *
 *   bus30ToPostEnumeratedBus()
 *
 *   This function is dedicated to converting bus 30 to post enumerated
 *   bus number for MMIO read.
 *
 ******************************************************************************/
static int bus30ToPostEnumeratedBus(uint32_t addr, uint8_t* postEnumBus)
{
    uint32_t cpubusno_valid = 0;
    uint32_t cpubusno2 = 0;
    uint8_t cc = 0;
    int ret = ACD_SUCCESS;

    // Use PCS Service 76, Parameter 5 to check valid post enumerated bus#
    ret = peci_RdPkgConfig(addr, 76, 5, sizeof(uint32_t),
                           (uint8_t*)&cpubusno_valid, &cc);
    if ((ret != PECI_CC_SUCCESS) || (PECI_CC_UA(cc)))
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Unable to read cpubusno_valid - cc: 0x%x ret: 0x%x\n",
                        cc, ret);
        // Need to return 1 for all failures
        return ACD_FAILURE;
    }

    // Bit 11 is for checking bus 30 contains valid post enumerated bus#
    if (0 == CHECK_BIT(cpubusno_valid, 11))
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Bus 30 does not contain valid post enumerated bus"
                        "number! (0x%x)\n",
                        cpubusno_valid);
        return ACD_FAILURE;
    }

    // Use PCS Service 76, Parameter 4 to get raw post enumerated buses value
    ret = peci_RdPkgConfig(addr, 76, 4, sizeof(uint32_t), (uint8_t*)&cpubusno2,
                           &cc);
    if ((ret != PECI_CC_SUCCESS) || (PECI_CC_UA(cc)))
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Unable to read cpubusno2 - cc: 0x%x\n ret: 0x%x\n", cc,
                        ret);
        // Need to return 1 for all failures
        return ACD_FAILURE;
    }

    // CPUBUSNO2[23:16] for Bus 30
    *postEnumBus = ((cpubusno2 >> 16) & 0xff);

    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   uncoreStatusRdIAMSRICXSPR
 *
 *   This function gathers the Uncore Status MSR using input file
 *
 ******************************************************************************/
int uncoreStatusRdIAMSRICXSPR(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    char jsonNameString[US_REG_NAME_LEN];
    int ret = 0;
    uint8_t cc = 0;

    cJSON* regList = NULL;
    cJSON* itRegs = NULL;
    cJSON* itParams = NULL;
    bool enable = false;

    regList = getCrashDataSectionRegList(cpuInfo->inputFile.bufferPtr, "uncore",
                                         "rdiamsr", &enable);

    if (regList == NULL)
    {
        cJSON_AddStringToObject(pJsonChild, FILE_RDIAMSR_KEY, FILE_RDIAMSR_ERR);
        return ACD_INVALID_OBJECT;
    }

    if (!enable)
    {
        cJSON_AddFalseToObject(pJsonChild, RECORD_ENABLE);
        return ACD_SUCCESS;
    }

    SUncoreStatusMsrRegICXSPR reg = {0};

    cJSON_ArrayForEach(itRegs, regList)
    {
        int position = 0;
        cJSON_ArrayForEach(itParams, itRegs)
        {
            switch (position)
            {
                case US_RDIAMSR_REG_NAME:
                    reg.regName = itParams->valuestring;
                    break;
                case US_RDIAMSR_ADDR:
                    reg.addr = strtoull(itParams->valuestring, NULL, 16);
                    break;
                case US_RDIAMSR_THREAD_ID:
                    reg.threadID = itParams->valueint;
                    break;
                case US_RDIAMSR_SIZE:
                    reg.u8Size = itParams->valueint;
                    break;
                default:
                    break;
            }
            position++;
        }
        SUncoreStatusRegRawData sRegData = {0};
        ret = peci_RdIAMSR(cpuInfo->clientAddr, reg.threadID, reg.addr,
                           &sRegData.uValue.u64, &cc);
        cd_snprintf_s(jsonNameString, US_JSON_STRING_LEN, reg.regName);
        if (reg.u8Size == US_REG_DWORD)
        {
            sRegData.uValue.u64 = sRegData.uValue.u64 & 0xFFFFFFFF;
        }
        uncoreStatusJsonICXSPR(jsonNameString, &sRegData, pJsonChild, cc, ret);
    }
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   uncoreStatusPciICXSPR
 *
 *   This function gathers the ICX/SPR Uncore Status PCI registers by using
 *   crashdump_input_icx.json
 *
 ******************************************************************************/
int uncoreStatusPciICXSPR(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int peci_fd = -1;
    int ret = 0;

    cJSON* regList = NULL;
    cJSON* itRegs = NULL;
    cJSON* itParams = NULL;
    bool enable = false;

    regList = getCrashDataSectionRegList(cpuInfo->inputFile.bufferPtr, "uncore",
                                         "pci", &enable);

    if (regList == NULL)
    {
        cJSON_AddStringToObject(pJsonChild, FILE_PCI_KEY, FILE_PCI_ERR);
        return ACD_INVALID_OBJECT;
    }

    if (!enable)
    {
        cJSON_AddFalseToObject(pJsonChild, RECORD_ENABLE);
        return ACD_SUCCESS;
    }

    ret = peci_Lock(&peci_fd, PECI_WAIT_FOREVER);
    if (ret != PECI_CC_SUCCESS)
    {
        return ret;
    }

    SUncoreStatusRegPciIcx pciReg = {0};

    cJSON_ArrayForEach(itRegs, regList)
    {
        int position = 0;
        cJSON_ArrayForEach(itParams, itRegs)
        {
            switch (position)
            {
                case US_PCI_REG_NAME:
                    pciReg.regName = itParams->valuestring;
                    break;
                case US_PCI_BUS:
                    pciReg.u8Bus = itParams->valueint;
                    break;
                case US_PCI_DEVICE:
                    pciReg.u8Dev = itParams->valueint;
                    break;
                case US_PCI_FUNCTION:
                    pciReg.u8Func = itParams->valueint;
                    break;
                case US_PCI_OFFSET:
                    pciReg.u16Reg = strtoull(itParams->valuestring, NULL, 16);
                    break;
                case US_PCI_SIZE:
                    pciReg.u8Size = itParams->valueint;
                    break;
                default:
                    break;
            }
            position++;
        }

        SUncoreStatusRegRawData sRegData = {0};
        uint8_t cc = 0;
        uint8_t bus = 0;

        // ICX EDS Reference Section: PCI Configuration Space Registers
        // Note that registers located in Bus 30 and 31
        // have been translated to Bus 13 and 14 respectively for PECI access.
        if (pciReg.u8Bus == 30)
        {
            bus = 13;
        }
        else if (pciReg.u8Bus == 31)
        {
            bus = 14;
        }
        else
        {
            bus = pciReg.u8Bus;
        }

        switch (pciReg.u8Size)
        {
            case US_REG_BYTE:
            case US_REG_WORD:
            case US_REG_DWORD:

                ret = peci_RdEndPointConfigPciLocal_seq(
                    cpuInfo->clientAddr, US_PCI_SEG, bus, pciReg.u8Dev,
                    pciReg.u8Func, pciReg.u16Reg, pciReg.u8Size,
                    (uint8_t*)&sRegData.uValue.u64, peci_fd, &cc);
                if (ret != PECI_CC_SUCCESS)
                {
                    sRegData.bInvalid = true;
                }
                break;
            case US_REG_QWORD:
                for (uint8_t u8Dword = 0; u8Dword < 2; u8Dword++)
                {
                    ret = peci_RdEndPointConfigPciLocal_seq(
                        cpuInfo->clientAddr, US_PCI_SEG, bus, pciReg.u8Dev,
                        pciReg.u8Func, pciReg.u16Reg + (u8Dword * 4),
                        sizeof(uint32_t),
                        (uint8_t*)&sRegData.uValue.u32[u8Dword], peci_fd, &cc);
                    if (ret != PECI_CC_SUCCESS)
                    {
                        sRegData.bInvalid = true;
                        break;
                    }
                }
                break;
            default:
                sRegData.bInvalid = true;
                ret = SIZE_FAILURE;
        }
        uncoreStatusJsonICXSPR(pciReg.regName, &sRegData, pJsonChild, cc, ret);
    }

    peci_Unlock(peci_fd);
    return ret;
}

/******************************************************************************
 *
 *   uncoreStatusPciMmioICXSPR
 *
 *   This function gathers the Uncore Status PCI MMIO registers by using
 *   crashdump_input_icx.json
 *
 ******************************************************************************/

int uncoreStatusPciMmioICXSPR(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    char jsonNameString[US_REG_NAME_LEN];
    int peci_fd = -1;
    int ret = 0;
    uint8_t cc = 0;
    uint8_t postEnumBus = 0;

    cJSON* itRegs = NULL;
    cJSON* itParams = NULL;
    cJSON* regList = NULL;
    bool enable = false;

    regList = getCrashDataSectionRegList(cpuInfo->inputFile.bufferPtr, "uncore",
                                         "mmio", &enable);

    if (regList == NULL)
    {
        cJSON_AddStringToObject(pJsonChild, FILE_MMIO_KEY, FILE_MMIO_ERR);
        return ACD_INVALID_OBJECT;
    }

    if (!enable)
    {
        cJSON_AddFalseToObject(pJsonChild, RECORD_ENABLE);
        return ACD_SUCCESS;
    }

    ret = peci_Lock(&peci_fd, PECI_WAIT_FOREVER);
    if (ret != PECI_CC_SUCCESS)
    {
        return ret;
    }

    if (0 != bus30ToPostEnumeratedBus(cpuInfo->clientAddr, &postEnumBus))
    {
        peci_Unlock(peci_fd);
        return ACD_FAILURE;
    }

    cd_snprintf_s(jsonNameString, US_JSON_STRING_LEN, "B%d", postEnumBus, cc);
    cJSON_AddStringToObject(pJsonChild, "_post_enumerated_B30", jsonNameString);

    SUncoreStatusRegPciMmioICXSPR mmioReg = {0};

    cJSON_ArrayForEach(itRegs, regList)
    {
        int position = 0;
        cJSON_ArrayForEach(itParams, itRegs)
        {
            switch (position)
            {
                case US_MMIO_REG_NAME:
                    mmioReg.regName = itParams->valuestring;
                    break;
                case US_MMIO_BAR_ID:
                    mmioReg.u8Bar = itParams->valueint;
                    break;
                case US_MMIO_BUS:
                    mmioReg.u8Bus = itParams->valueint;
                    break;
                case US_MMIO_DEVICE:
                    mmioReg.u8Dev = itParams->valueint;
                    break;
                case US_MMIO_FUNCTION:
                    mmioReg.u8Func = itParams->valueint;
                    break;
                case US_MMIO_OFFSET:
                    mmioReg.u64Offset =
                        strtoull(itParams->valuestring, NULL, 16);
                    break;
                case US_MMIO_ADDRTYPE:
                    mmioReg.u8AddrType = itParams->valueint;
                    break;
                case US_MMIO_SIZE:
                    mmioReg.u8Size = itParams->valueint;
                    break;
                default:
                    break;
            }
            position++;
        }

        SUncoreStatusRegRawData sRegData = {0};
        uint8_t readLen = 0;

        switch (mmioReg.u8Size)
        {
            case US_REG_BYTE:
            case US_REG_WORD:
            case US_REG_DWORD:
                readLen = US_REG_DWORD;
                break;
            case US_REG_QWORD:
                readLen = US_REG_QWORD;
                break;
            default:
                sRegData.bInvalid = true;
                ret = SIZE_FAILURE;
        }

        ret = peci_RdEndPointConfigMmio_seq(
            cpuInfo->clientAddr, US_MMIO_SEG, postEnumBus, mmioReg.u8Dev,
            mmioReg.u8Func, mmioReg.u8Bar, mmioReg.u8AddrType,
            mmioReg.u64Offset, readLen, (uint8_t*)&sRegData.uValue.u64, peci_fd,
            &cc);
        if (ret != PECI_CC_SUCCESS)
        {
            sRegData.bInvalid = true;
        }

        uncoreStatusJsonICXSPR(mmioReg.regName, &sRegData, pJsonChild, cc, ret);
    }

    peci_Unlock(peci_fd);
    return ret;
}

static UncoreStatusRead UncoreStatusTypesICXSPR[] = {
    uncoreStatusPciICXSPR,
    uncoreStatusPciMmioICXSPR,
    uncoreStatusRdIAMSRICXSPR,
};

/******************************************************************************
 *
 *   logUncoreStatusICXSPR
 *
 *   This function gathers the Uncore Status register contents and adds them to
 *   the debug log.
 *
 ******************************************************************************/
int logUncoreStatusICXSPR(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int ret = 0;

    for (uint32_t i = 0; i < (sizeof(UncoreStatusTypesICXSPR) /
                              sizeof(UncoreStatusTypesICXSPR[0]));
         i++)
    {
        if (UncoreStatusTypesICXSPR[i](cpuInfo, pJsonChild) != 0)
        {
            ret = 1;
        }
    }

    return ret;
}

static const SUncoreStatusLogVx sUncoreStatusLogVx[] = {
    {cd_icx, logUncoreStatusICXSPR},
    {cd_icx2, logUncoreStatusICXSPR},
    {cd_icxd, logUncoreStatusICXSPR},
    {cd_spr, logUncoreStatusICXSPR},
};

/******************************************************************************
 *
 *   logUncoreStatus
 *
 *   This function gathers the Uncore Status register contents and adds them to
 *   the debug log.
 *
 ******************************************************************************/
int logUncoreStatus(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    for (uint32_t i = 0;
         i < (sizeof(sUncoreStatusLogVx) / sizeof(SUncoreStatusLogVx)); i++)
    {
        if (cpuInfo->model == sUncoreStatusLogVx[i].cpuModel)
        {
            revision_uncore = getCrashDataSectionVersion(
                cpuInfo->inputFile.bufferPtr, "uncore");
            logCrashdumpVersion(pJsonChild, cpuInfo,
                                RECORD_TYPE_UNCORESTATUSLOG);
            return sUncoreStatusLogVx[i].logUncoreStatusVx(cpuInfo, pJsonChild);
        }
    }

    CRASHDUMP_PRINT(ERR, stderr, "Cannot find version for %s\n", __FUNCTION__);
    return ACD_FAILURE;
}