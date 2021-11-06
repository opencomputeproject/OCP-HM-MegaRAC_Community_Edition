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

#include "AddressMap.h"

#include "utils.h"

/******************************************************************************
 *
 *   addressMapJsonICX
 *
 *   This function formats the Address Map PCI registers into a JSON object
 *
 ******************************************************************************/
static void addressMapJsonICX(const char* regName,
                              SAddressMapRegRawData* sRegData,
                              cJSON* pJsonChild, uint8_t cc, int ret)
{
    char jsonItemString[AM_JSON_STRING_LEN];

    if (sRegData->bInvalid)
    {
        cd_snprintf_s(jsonItemString, AM_JSON_STRING_LEN, AM_FIXED_DATA_CC_RC,
                      cc, ret);
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, AM_JSON_STRING_LEN, AM_DATA_CC_RC,
                      sRegData->uValue.u64, cc, ret);
    }
    else
    {
        cd_snprintf_s(jsonItemString, AM_JSON_STRING_LEN, AM_UINT64_FMT,
                      sRegData->uValue.u64);
    }

    cJSON_AddStringToObject(pJsonChild, regName, jsonItemString);
}

/******************************************************************************
 *
 *   logAddressMapEntriesICX1
 *
 *   This function gathers the Address Map PCI registers
 *
 ******************************************************************************/
int logAddressMapEntriesICX1(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int peci_fd = -1;
    int ret = 0;

    cJSON* regList = NULL;
    cJSON* itRegs = NULL;
    cJSON* itParams = NULL;
    regList =
        getCrashDataSectionAddressMapRegList(cpuInfo->inputFile.bufferPtr);

    if (regList == NULL)
    {
        cJSON_AddStringToObject(pJsonChild, FILE_ADDRESS_MAP_KEY,
                                FILE_ADDRESS_MAP_ERR);
        return ACD_INVALID_OBJECT;
    }

    ret = peci_Lock(&peci_fd, PECI_WAIT_FOREVER);
    if (ret != PECI_CC_SUCCESS)
    {
        return ret;
    }

    SAddrMapEntry addrMapRegs = {0};
    cJSON_ArrayForEach(itRegs, regList)
    {
        int position = 0;
        cJSON_ArrayForEach(itParams, itRegs)
        {
            switch (position)
            {
                case ADDRESS_MAP_REG_NAME:
                    addrMapRegs.regName = itParams->valuestring;
                    break;
                case ADDRESS_MAP_BUS:
                    addrMapRegs.u8Bus = itParams->valueint;
                    break;
                case ADDRESS_MAP_DEVICE:
                    addrMapRegs.u8Dev = itParams->valueint;
                    break;
                case ADDRESS_MAP_FUNCTION:
                    addrMapRegs.u8Func = itParams->valueint;
                    break;
                case ADDRESS_MAP_OFFSET:
                    addrMapRegs.u16Reg =
                        strtoull(itParams->valuestring, NULL, 16);
                    break;
                case ADDRESS_MAP_SIZE:
                    addrMapRegs.u8Size = itParams->valueint;
                    break;
                default:
                    break;
            }
            position++;
        }

        SAddressMapRegRawData sRegData = {0};
        uint8_t cc = 0;
        uint8_t bus = 0;
        uint8_t ccVal = PECI_DEV_CC_SUCCESS;

        // ICX EDS Reference Section: PCI Configuration Space Registers
        // Note that registers located in Bus 30 and 31
        // have been translated to Bus 13 and 14 respectively for PECI access.
        if (addrMapRegs.u8Bus == 30)
        {
            bus = 13;
        }
        else if (addrMapRegs.u8Bus == 31)
        {
            bus = 14;
        }
        else
        {
            bus = addrMapRegs.u8Bus;
        }

        switch (addrMapRegs.u8Size)
        {
            case AM_REG_BYTE:
            case AM_REG_WORD:
            case AM_REG_DWORD:
                ret = peci_RdEndPointConfigPciLocal_seq(
                    cpuInfo->clientAddr, AM_PCI_SEG, bus, addrMapRegs.u8Dev,
                    addrMapRegs.u8Func, addrMapRegs.u16Reg, addrMapRegs.u8Size,
                    (uint8_t*)&sRegData.uValue.u64, peci_fd, &cc);
                if (ret != PECI_CC_SUCCESS)
                {
                    sRegData.bInvalid = true;
                }
                break;
            case AM_REG_QWORD:
                for (uint8_t u8Dword = 0; u8Dword < 2; u8Dword++)
                {
                    ret = peci_RdEndPointConfigPciLocal_seq(
                        cpuInfo->clientAddr, AM_PCI_SEG, bus, addrMapRegs.u8Dev,
                        addrMapRegs.u8Func, addrMapRegs.u16Reg + (u8Dword * 4),
                        sizeof(uint32_t),
                        (uint8_t*)&sRegData.uValue.u32[u8Dword], peci_fd, &cc);

                    if (PECI_CC_UA(cc))
                    {
                        ccVal = cc;
                    }
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
        if (PECI_CC_UA(ccVal))
        {
            cc = ccVal;
        }
        addressMapJsonICX(addrMapRegs.regName, &sRegData, pJsonChild, cc, ret);
    }

    peci_Unlock(peci_fd);
    return ret;
}

/******************************************************************************
 *
 *   logAddressMapICX1
 *
 *   This function logs the ICX1 Address Map
 *
 ******************************************************************************/
int logAddressMapICX1(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    return logAddressMapEntriesICX1(cpuInfo, pJsonChild);
}

static const SAddrMapVx sAddrMapVx[] = {
    {cd_icx, logAddressMapICX1},
    {cd_icx2, logAddressMapICX1},
    {cd_icxd, logAddressMapICX1},
};

/******************************************************************************
 *
 *   logAddressMap
 *
 *   This function gathers the Address Map log and adds it to the debug log
 *
 ******************************************************************************/
int logAddressMap(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    for (uint32_t i = 0; i < (sizeof(sAddrMapVx) / sizeof(SAddrMapVx)); i++)
    {
        if (cpuInfo->model == sAddrMapVx[i].cpuModel)
        {
            return sAddrMapVx[i].logAddrMapVx(cpuInfo, pJsonChild);
        }
    }

    CRASHDUMP_PRINT(ERR, stderr, "Cannot find version for %s\n", __FUNCTION__);
    return ACD_FAILURE;
}
