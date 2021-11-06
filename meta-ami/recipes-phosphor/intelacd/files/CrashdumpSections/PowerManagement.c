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

#include "PowerManagement.h"

#include "utils.h"

/******************************************************************************
 *
 *   powerManagementJsonICX1
 *
 *   This function formats the Power Management log into a JSON object
 *
 ******************************************************************************/
static void powerManagementJsonICX1(const char* regName,
                                    SPmRegRawData* sRegData, cJSON* pJsonChild,
                                    uint8_t cc, int ret)
{
    char jsonItemString[PM_JSON_STRING_LEN] = {0};

    if (sRegData->bInvalid)
    {
        cd_snprintf_s(jsonItemString, PM_JSON_STRING_LEN, PM_FIXED_DATA_CC_RC,
                      cc, ret);
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, PM_JSON_STRING_LEN, PM_DATA_CC_RC,
                      sRegData->uValue, cc, ret);
    }
    else
    {
        cd_snprintf_s(jsonItemString, PM_JSON_STRING_LEN, PM_UINT32_FMT,
                      sRegData->uValue, cc);
    }

    cJSON_AddStringToObject(pJsonChild, regName, jsonItemString);
}

/******************************************************************************
 *
 *   powerManagementPllJsonICX1
 *
 *   This function formats the Pll log into a JSON object
 *   It uses section name and regName and generates 2 levels.
 *
 ******************************************************************************/
static void powerManagementPllJsonICX1(const char* secName, const char* regName,
                                       SPmRegRawData* sRegData,
                                       cJSON* pJsonChild, uint8_t cc, int ret)
{
    char jsonItemString[PM_JSON_STRING_LEN] = {0};
    cJSON* section = NULL;
    if ((section = cJSON_GetObjectItemCaseSensitive(pJsonChild, secName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, secName,
                              section = cJSON_CreateObject());
    }
    if (sRegData->bInvalid)
    {
        cd_snprintf_s(jsonItemString, PM_JSON_STRING_LEN, PM_FIXED_DATA_CC_RC,
                      cc, ret);
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, PM_JSON_STRING_LEN, PM_DATA_CC_RC,
                      sRegData->uValue, cc, ret);
    }
    else
    {

        cd_snprintf_s(jsonItemString, PM_JSON_STRING_LEN, PM_UINT32_FMT,
                      sRegData->uValue, cc);
    }

    cJSON_AddStringToObject(section, regName, jsonItemString);
}

/******************************************************************************
 *
 *   powerManagementJsonPerCoreICX1
 *
 *   This function formats Power Management Per Core log into a JSON object
 *
 ******************************************************************************/
static void powerManagementJsonPerCoreICX1(const char* regName,
                                           uint32_t u32CoreNum,
                                           SPmRegRawData* sRegData,
                                           cJSON* pJsonChild, uint8_t cc,
                                           int ret)
{
    char jsonItemName[PM_JSON_STRING_LEN] = {0};
    cJSON* core = NULL;

    // Add the core number item to the Power Management JSON structure
    cd_snprintf_s(jsonItemName, PM_JSON_STRING_LEN, PM_JSON_CORE_NAME,
                  u32CoreNum);

    if ((core = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              core = cJSON_CreateObject());
    }

    powerManagementJsonICX1(regName, sRegData, core, cc, ret);
}

/******************************************************************************
 *
 *   logPowerManagementICX1
 *
 *   This function reads the input file and gathers the Power Management log
 *   and adds it to the debug log.
 *   The PECI flow is listed below to dump the core state registers
 *
 ******************************************************************************/
int logPowerManagementICX1(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int ret = 0;
    uint8_t cc = 0;
    char jsonNameString[PM_JSON_STRING_LEN];
    cJSON* regList = NULL;
    cJSON* itRegs = NULL;
    cJSON* itParams = NULL;
    bool enable = false;

    regList = getCrashDataSectionRegList(cpuInfo->inputFile.bufferPtr,
                                         "PM_info", "pci", &enable);

    if (regList == NULL)
    {
        cJSON_AddStringToObject(pJsonChild, FILE_PM_KEY, FILE_PM_ERR);
        return ACD_INVALID_OBJECT;
    }

    if (!enable)
    {
        cJSON_AddFalseToObject(pJsonChild, RECORD_ENABLE);
        return ACD_SUCCESS;
    }

    SPmEntry reg = {0};
    cJSON_ArrayForEach(itRegs, regList)
    {
        int position = 0;
        cJSON_ArrayForEach(itParams, itRegs)

        {
            switch (position)
            {
                case PM_REG_NAME:
                    reg.regName = itParams->valuestring;
                    break;
                case PM_CORESCOPE:
                    reg.coreScope = itParams->valueint;
                    break;
                case PM_PARAM:
                    reg.param = strtoull(itParams->valuestring, NULL, 16);
                    break;
                default:
                    break;
            }
            position++;
        }
        SPmRegRawData sRegData = {0};
        if (reg.coreScope)
        {
            // Go through each enabled core
            for (uint32_t u32CoreNum = 0;
                 (cpuInfo->coreMask >> u32CoreNum) != 0; u32CoreNum++)
            {
                if (!CHECK_BIT(cpuInfo->coreMask, u32CoreNum))
                {
                    continue;
                }

                uint16_t param = (u32CoreNum << 8) | (uint8_t)reg.param;

                ret = peci_RdPkgConfig(cpuInfo->clientAddr, PM_PCS_88, param,
                                       sizeof(uint32_t),
                                       (uint8_t*)&sRegData.uValue, &cc);
                if (ret != PECI_CC_SUCCESS)
                {
                    sRegData.bInvalid = true;
                }
                cd_snprintf_s(jsonNameString, PM_JSON_STRING_LEN, reg.regName);
                powerManagementJsonPerCoreICX1(jsonNameString, u32CoreNum,
                                               &sRegData, pJsonChild, cc, ret);
            }
        }
        else
        {
            ret = peci_RdPkgConfig(cpuInfo->clientAddr, PM_PCS_88, reg.param,
                                   sizeof(uint32_t), (uint8_t*)&sRegData.uValue,
                                   &cc);
            if (ret != PECI_CC_SUCCESS)
            {
                sRegData.bInvalid = true;
            }
            cd_snprintf_s(jsonNameString, PM_JSON_STRING_LEN, reg.regName);
            powerManagementJsonICX1(jsonNameString, &sRegData, pJsonChild, cc,
                                    ret);
        }
    }
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   logPllCX1
 *
 *   This function reads the input file and gathers the Phase Lock Loop log
 *   and adds it to the debug log.
 *   The PECI flow is listed below to dump the core state registers
 *
 ******************************************************************************/
int logPllICX1(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int ret = 0;
    uint8_t cc = 0;
    char jsonNameString1[PM_JSON_STRING_LEN];
    char jsonNameString2[PM_JSON_STRING_LEN];
    cJSON* regList = NULL;
    cJSON* itRegs = NULL;
    cJSON* itParams = NULL;
    bool enable = false;
    regList = getCrashDataSectionRegList(cpuInfo->inputFile.bufferPtr,
                                         "PM_info", "pll", &enable);

    if (regList == NULL)
    {
        cJSON_AddStringToObject(pJsonChild, FILE_PLL_KEY, FILE_PLL_ERR);
        return ACD_INVALID_OBJECT;
    }

    if (!enable)
    {
        cJSON_AddFalseToObject(pJsonChild, RECORD_ENABLE);
        return ACD_SUCCESS;
    }

    SPmEntry reg = {0};

    cJSON_ArrayForEach(itRegs, regList)
    {
        int position = 0;
        cJSON_ArrayForEach(itParams, itRegs)
        {
            switch (position)
            {
                case PLL_SEC_NAME:
                    reg.secName = itParams->valuestring;
                    break;
                case PLL_REG_NAME:
                    reg.regName = itParams->valuestring;
                    break;
                case PLL_PARAM:
                    reg.param = strtoull(itParams->valuestring, NULL, 16);
                    break;
                default:
                    break;
            }
            position++;
        }
        SPmRegRawData sRegData = {0};
        ret =
            peci_RdPkgConfig(cpuInfo->clientAddr, PM_PCS_88, reg.param,
                             sizeof(uint32_t), (uint8_t*)&sRegData.uValue, &cc);
        if (ret != PECI_CC_SUCCESS)
        {
            sRegData.bInvalid = true;
        }
        cd_snprintf_s(jsonNameString1, PM_JSON_STRING_LEN, reg.regName);
        cd_snprintf_s(jsonNameString2, PM_JSON_STRING_LEN, reg.secName);
        powerManagementPllJsonICX1(jsonNameString2, jsonNameString1, &sRegData,
                                   pJsonChild, cc, ret);
    }
    return ACD_SUCCESS;
}

static PowerManagementStatusRead PowerManagementStatusTypesICX[] = {
    logPowerManagementICX1,
    logPllICX1,
};

int logPowerManagementICX(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int ret = 0;
    for (uint32_t i = 0; i < (sizeof(PowerManagementStatusTypesICX) /
                              sizeof(PowerManagementStatusTypesICX[0]));
         i++)
    {
        if (PowerManagementStatusTypesICX[i](cpuInfo, pJsonChild) != 0)
        {
            ret = 1;
        }
    }

    return ret;
}

static const SPowerManagementVx sPowerManagementVx[] = {
    {cd_icx, logPowerManagementICX},
    {cd_icx2, logPowerManagementICX},
    {cd_icxd, logPowerManagementICX},
};

/******************************************************************************
 *
 *   logPowerManagement
 *
 *   This function gathers the PowerManagement log and adds it to the debug log
 *
 ******************************************************************************/
int logPowerManagement(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    for (uint32_t i = 0;
         i < (sizeof(sPowerManagementVx) / sizeof(SPowerManagementVx)); i++)
    {
        if (cpuInfo->model == sPowerManagementVx[i].cpuModel)
        {
            return sPowerManagementVx[i].logPowerManagementVx(cpuInfo,
                                                              pJsonChild);
        }
    }

    CRASHDUMP_PRINT(ERR, stderr, "Cannot find version for %s\n", __FUNCTION__);
    return ACD_FAILURE;
}
