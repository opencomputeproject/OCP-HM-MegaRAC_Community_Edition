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

#include "CoreMca.h"

#include "utils.h"

/******************************************************************************
 *
 *   coreMcaJson
 *
 *   This function formats the Core MCA into a JSON object
 *
 ******************************************************************************/
static void coreMcaJson(uint32_t u32CoreNum, uint32_t u32ThreadNum,
                        const SCoreMcaReg* sCoreMcaReg, uint64_t u64CoreMcaData,
                        cJSON* pJsonChild, uint8_t cc, int ret, bool isValid)
{
    char jsonItemString[CORE_MCA_JSON_STRING_LEN];

    // Add the core number item to the Core MCA JSON structure only if it
    // doesn't already exist
    cJSON* core;
    cd_snprintf_s(jsonItemString, CORE_MCA_JSON_STRING_LEN,
                  CORE_MCA_JSON_CORE_NAME, u32CoreNum);
    if ((core = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemString)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemString,
                              core = cJSON_CreateObject());
    }

    // Add the thread number item to the Core MCA JSON structure only if it
    // doesn't already exist
    cJSON* thread;
    cd_snprintf_s(jsonItemString, CORE_MCA_JSON_STRING_LEN,
                  CORE_MCA_JSON_THREAD_NAME, u32ThreadNum);
    if ((thread = cJSON_GetObjectItemCaseSensitive(core, jsonItemString)) ==
        NULL)
    {
        cJSON_AddItemToObject(core, jsonItemString,
                              thread = cJSON_CreateObject());
    }

    // Add the MCA number item to the Core MCA JSON structure only if it
    // doesn't already exist
    cJSON* coreMca;
    if ((coreMca = cJSON_GetObjectItemCaseSensitive(
             thread, sCoreMcaReg->bankName)) == NULL)
    {
        cJSON_AddItemToObject(thread, sCoreMcaReg->bankName,
                              coreMca = cJSON_CreateObject());
    }

    if (!isValid)
    {
        cd_snprintf_s(jsonItemString, CORE_MCA_JSON_STRING_LEN, CORE_MCA_NA,
                      cc);
        cJSON_AddStringToObject(coreMca, sCoreMcaReg->regName, jsonItemString);
        return;
    }

    // Add the MCA register data to the Core MCA JSON structure
    if (ret != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, CORE_MCA_JSON_STRING_LEN,
                      CORE_MCA_FIXED_DATA_CC_RC, cc, ret);
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, CORE_MCA_JSON_STRING_LEN,
                      CORE_MCA_DATA_CC_RC, u64CoreMcaData, cc, ret);
    }
    else
    {
        // Add the MCA register data to the Core MCA JSON structure
        cd_snprintf_s(jsonItemString, CORE_MCA_JSON_STRING_LEN,
                      CORE_MCA_UINT64_FMT, u64CoreMcaData);
    }

    // Remove initial N/A entry
    cJSON* tmp = NULL;
    tmp = cJSON_GetObjectItemCaseSensitive(coreMca, sCoreMcaReg->regName);
    if (tmp != NULL)
    {
        cJSON_DeleteItemFromObjectCaseSensitive(coreMca, tmp->string);
    }
    cJSON_AddStringToObject(coreMca, sCoreMcaReg->regName, jsonItemString);
}

static void readParams(SCoreMcaReg* reg, const cJSON* itRegs)
{
    int position = 0;
    cJSON* itParams = NULL;
    cJSON_ArrayForEach(itParams, itRegs)
    {
        switch (position)
        {
            case MCA_CORE_BANK_NAME:
                reg->bankName = itParams->valuestring;
                break;
            case MCA_CORE_REG_NAME:
                reg->regName = itParams->valuestring;
                break;
            case MCA_CORE_ADDR:
                reg->addr = strtoull(itParams->valuestring, NULL, 16);
                break;
            default:
                break;
        }
        position++;
    }
}

/******************************************************************************
 *
 *   logCoreMcaICXSPR
 *
 *   This function gathers the Core MCA register contents and adds them to the
 *   debug log.
 *
 ******************************************************************************/
int logCoreMcaICXSPR(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int ret = ACD_FAILURE;
    cJSON* regList = NULL;
    cJSON* itRegs = NULL;
    bool enable = false;

    regList = getCrashDataSectionRegList(cpuInfo->inputFile.bufferPtr, "MCA",
                                         "core", &enable);

    if (regList == NULL)
    {
        cJSON_AddStringToObject(pJsonChild, FILE_MCA_KEY, FILE_MCA_ERR);
        return ACD_INVALID_OBJECT;
    }

    if (!enable)
    {
        cJSON_AddFalseToObject(pJsonChild, RECORD_ENABLE);
        return ACD_SUCCESS;
    }

    SCoreMcaReg reg = {0};

    // Go through each enabled core
    for (uint32_t u32CoreNum = 0; (cpuInfo->coreMask >> u32CoreNum) != 0;
         u32CoreNum++)
    {
        if (!CHECK_BIT(cpuInfo->coreMask, u32CoreNum))
        {
            continue;
        }

        if (CHECK_BIT(cpuInfo->crashedCoreMask, u32CoreNum))
        {
            continue;
        }

        // Go through each thread on this core
        for (uint32_t u32ThreadNum = 0;
             u32ThreadNum < CORE_MCA_THREADS_PER_CORE; u32ThreadNum++)
        {
            uint64_t u64CoreMcaData = 0;
            uint8_t cc = 0;

            // Fill json with N/A first
            cJSON_ArrayForEach(itRegs, regList)
            {
                readParams(&reg, itRegs);
                coreMcaJson(u32CoreNum, u32ThreadNum, &reg, 0, pJsonChild, 0,
                            ret, !CORE_MCA_VALID);
            }

            // Go through each core register on this thread and log it
            cJSON_ArrayForEach(itRegs, regList)
            {
                readParams(&reg, itRegs);
                u64CoreMcaData = 0;
                cc = 0;
                ret = peci_RdIAMSR(cpuInfo->clientAddr,
                                   (u32CoreNum * 2) + u32ThreadNum, reg.addr,
                                   &u64CoreMcaData, &cc);

                if (PECI_CC_SKIP_CORE(cc))
                {
                    coreMcaJson(u32CoreNum, u32ThreadNum, &reg, u64CoreMcaData,
                                pJsonChild, cc, ret, CORE_MCA_VALID);
                    goto nextCore;
                }

                if (PECI_CC_SKIP_SOCKET(cc))
                {
                    coreMcaJson(u32CoreNum, u32ThreadNum, &reg, u64CoreMcaData,
                                pJsonChild, cc, ret, CORE_MCA_VALID);
                    return ret;
                }

                // Log the MCA register
                coreMcaJson(u32CoreNum, u32ThreadNum, &reg, u64CoreMcaData,
                            pJsonChild, cc, ret, CORE_MCA_VALID);
            }
        }
    nextCore:;
    }
    return ACD_SUCCESS;
}

static const SCoreMcaLogVx sCoreMcaLogVx[] = {
    {cd_icx, logCoreMcaICXSPR},
    {cd_icx2, logCoreMcaICXSPR},
    {cd_spr, logCoreMcaICXSPR},
    {cd_icxd, logCoreMcaICXSPR},
};

/******************************************************************************
 *
 *   logCoreMca
 *
 *   This function gathers the Core MCA register contents and adds them to the
 *   debug log.
 *
 ******************************************************************************/
int logCoreMca(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    for (uint32_t i = 0; i < (sizeof(sCoreMcaLogVx) / sizeof(SCoreMcaLogVx));
         i++)
    {
        if (cpuInfo->model == sCoreMcaLogVx[i].cpuModel)
        {
            return sCoreMcaLogVx[i].logCoreMcaVx(cpuInfo, pJsonChild);
        }
    }

    CRASHDUMP_PRINT(ERR, stderr, "Cannot find version for %s\n", __FUNCTION__);
    return ACD_FAILURE;
}
