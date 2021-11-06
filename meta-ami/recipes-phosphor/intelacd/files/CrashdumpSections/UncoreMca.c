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

#include "UncoreMca.h"

#include "utils.h"

static void readCboParams(SUncoreMcaCboReg* reg, const cJSON* itRegs)
{
    int position = 0;
    cJSON* itParams = NULL;
    cJSON_ArrayForEach(itParams, itRegs)
    {
        switch (position)
        {
            case MCA_UC_CBO_REG_NAME:
                reg->regName = itParams->valuestring;
                break;
            case MCA_UC_CBO_ADDR:
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
 *   coreMcaCboJson
 *
 *   This function formats the core MCA CBO registers into a JSON
 *   object
 *
 ******************************************************************************/
static void unCoreMcaCboJson(uint32_t u32CboNum, SUncoreMcaCboReg* reg,
                             cJSON* pJsonChild, int ret)
{
    char jsonItemString[UNCORE_MCA_JSON_STRING_LEN];
    char jsonNameString[UNCORE_MCA_JSON_STRING_LEN];

    // Add the uncore item to the Uncore MCA JSON structure only if it doesn't
    // already exist
    cJSON* uncore;
    cd_snprintf_s(jsonItemString, UNCORE_MCA_JSON_STRING_LEN,
                  UNCORE_MCA_JSON_SECTION_NAME);
    if ((uncore = cJSON_GetObjectItemCaseSensitive(pJsonChild,
                                                   jsonItemString)) == NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemString,
                              uncore = cJSON_CreateObject());
    }

    // Format the Uncore Status CBO MCA data out to the .json debug file
    // Fill in NULL for this CBO MCA if it's not valid
    cJSON* uncoreMcaCbo;
    cd_snprintf_s(jsonNameString, UNCORE_MCA_JSON_STRING_LEN,
                  UNCORE_MCA_JSON_CBO_NAME, u32CboNum, reg->regName);
    if ((uncoreMcaCbo =
             cJSON_GetObjectItemCaseSensitive(uncore, jsonNameString)) == NULL)
    {
        cd_snprintf_s(jsonItemString, UNCORE_MCA_JSON_STRING_LEN,
                      UNCORE_MCA_JSON_CBO_NAME, u32CboNum);
        cJSON_AddItemToObject(uncore, jsonItemString,
                              uncoreMcaCbo = cJSON_CreateObject());
    }

    if (!reg->valid)
    {
        cd_snprintf_s(jsonNameString, UNCORE_MCA_JSON_STRING_LEN,
                      UNCORE_MCA_CBO_REG_NAME, u32CboNum, reg->regName);
        cd_snprintf_s(jsonItemString, UNCORE_MCA_JSON_STRING_LEN,
                      UNCORE_MCA_FIXED_DATA_CC_RC, reg->cc, ret);
        cJSON_AddStringToObject(uncoreMcaCbo, jsonNameString, jsonItemString);
    }
    else if (PECI_CC_UA(reg->cc))
    {
        cd_snprintf_s(jsonNameString, UNCORE_MCA_JSON_STRING_LEN,
                      UNCORE_MCA_CBO_REG_NAME, u32CboNum, reg->regName);
        cd_snprintf_s(jsonItemString, UNCORE_MCA_JSON_STRING_LEN,
                      UNCORE_MCA_DATA_CC_RC, reg->data, reg->cc, ret);
        cJSON_AddStringToObject(uncoreMcaCbo, jsonNameString, jsonItemString);
    }
    else
    {
        cd_snprintf_s(jsonNameString, UNCORE_MCA_JSON_STRING_LEN,
                      UNCORE_MCA_CBO_REG_NAME, u32CboNum, reg->regName);
        cd_snprintf_s(jsonItemString, UNCORE_MCA_JSON_STRING_LEN, "0x%llx",
                      reg->data);
        cJSON_AddStringToObject(uncoreMcaCbo, jsonNameString, jsonItemString);
    }
}

/******************************************************************************
 *
 *   uncoreMcaJsonICXSPR
 *
 *   This function formats the Uncore MCA into a JSON object
 *
 ******************************************************************************/
static void uncoreMcaJsonICXSPR(const SUncoreMcaReg* sUncoreMcaReg,
                                uint64_t u64UncoreMcaData, cJSON* pJsonChild,
                                uint8_t cc, int ret)
{
    char jsonItemString[UNCORE_MCA_JSON_STRING_LEN];

    // Add the uncore item to the Uncore MCA JSON structure only if it doesn't
    // already exist
    cJSON* uncore;
    cd_snprintf_s(jsonItemString, UNCORE_MCA_JSON_STRING_LEN,
                  UNCORE_MCA_JSON_SECTION_NAME);
    if ((uncore = cJSON_GetObjectItemCaseSensitive(pJsonChild,
                                                   jsonItemString)) == NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemString,
                              uncore = cJSON_CreateObject());
    }

    // Add the MCA number item to the Uncore MCA JSON structure only if it
    // doesn't already exist
    cJSON* uncoreMca;
    if ((uncoreMca = cJSON_GetObjectItemCaseSensitive(
             uncore, sUncoreMcaReg->bankName)) == NULL)
    {
        cJSON_AddItemToObject(uncore, sUncoreMcaReg->bankName,
                              uncoreMca = cJSON_CreateObject());
    }
    if (ret != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, UNCORE_MCA_JSON_STRING_LEN,
                      UNCORE_MCA_FIXED_DATA_CC_RC, cc, ret);
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, UNCORE_MCA_JSON_STRING_LEN,
                      UNCORE_MCA_DATA_CC_RC, u64UncoreMcaData, cc, ret);
    }
    else
    {
        cd_snprintf_s(jsonItemString, UNCORE_MCA_JSON_STRING_LEN, "0x%llx",
                      u64UncoreMcaData);
    }

    // Add the MCA register data to the Uncore MCA JSON structure
    cJSON_AddStringToObject(uncoreMca, sUncoreMcaReg->regName, jsonItemString);
}

/******************************************************************************
 *
 *   logUnCoreMcaCboICXSPR
 *
 *   This function gathers the UnCore MCA CBO registers
 *
 ******************************************************************************/
static int logUnCoreMcaCboICXSPR(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int ret = ACD_FAILURE;
    cJSON* regList = NULL;
    cJSON* chaCountJson = NULL;
    uint8_t chaCount = cpuInfo->chaCount;
    cJSON* itRegs = NULL;
    bool enable = false;

    regList = getCrashDataSectionRegList(cpuInfo->inputFile.bufferPtr, "MCA",
                                         "cbo", &enable);

    if (regList == NULL)
    {
        cJSON_AddStringToObject(pJsonChild, FILE_MCA_UC_KEY, FILE_MCA_UC_ERR);
        return ACD_INVALID_OBJECT;
    }

    // Override cha count if override_cha_count key exists in the input file.
    chaCountJson =
        getCrashDataSectionObject(cpuInfo->inputFile.bufferPtr, "MCA", "cbo",
                                  "override_cha_count", &enable);
    if (chaCountJson != NULL)
    {
        chaCount = chaCountJson->valueint;
    }

    for (size_t i = 0; i < chaCount; i++)
    {
        SUncoreMcaCboReg reg = {0};
        reg.valid = true;

        cJSON_ArrayForEach(itRegs, regList)
        {
            readCboParams(&reg, itRegs);
            ret = peci_RdIAMSR(cpuInfo->clientAddr, i, reg.addr, &reg.data,
                               &reg.cc);
            if (ret != PECI_CC_SUCCESS)
            {
                reg.valid = false;
            }
            unCoreMcaCboJson(i, &reg, pJsonChild, ret);
        }
    }

    return ret;
}

static void readParams(SUncoreMcaReg* reg, const cJSON* itRegs)
{
    int position = 0;
    cJSON* itParams = NULL;
    cJSON_ArrayForEach(itParams, itRegs)
    {
        switch (position)
        {
            case MCA_UC_BANK_NAME:
                reg->bankName = itParams->valuestring;
                break;
            case MCA_UC_REG_NAME:
                reg->regName = itParams->valuestring;
                break;
            case MCA_UC_ADDR:
                reg->addr = strtoull(itParams->valuestring, NULL, 16);
                break;
            case MCA_UC_ID:
                reg->instance_id = itParams->valueint;
                break;
            default:
                break;
        }
        position++;
    }
}

/******************************************************************************
 *
 *   logUncoreMcaICXSPR
 *
 *   This function gathers the Uncore MCA register contents and adds them to the
 *   debug log.
 *
 ******************************************************************************/
int logUncoreMcaICXSPR(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int ret = ACD_FAILURE;
    cJSON* regList = NULL;
    cJSON* itRegs = NULL;
    bool enable = false;
    uint8_t cc = 0;

    regList = getCrashDataSectionRegList(cpuInfo->inputFile.bufferPtr, "MCA",
                                         "uncore", &enable);

    if (regList == NULL)
    {
        cJSON_AddStringToObject(pJsonChild, FILE_MCA_UC_KEY, FILE_MCA_UC_ERR);
        return ACD_INVALID_OBJECT;
    }

    SUncoreMcaReg reg = {0};

    // Go through each uncore register on this cpu and log it
    cJSON_ArrayForEach(itRegs, regList)
    {
        uint64_t u64UncoreMcaData = 0;
        readParams(&reg, itRegs);
        ret = peci_RdIAMSR(cpuInfo->clientAddr, reg.instance_id, reg.addr,
                           &u64UncoreMcaData, &cc);
        uncoreMcaJsonICXSPR(&reg, u64UncoreMcaData, pJsonChild, cc, ret);
    }

    logUnCoreMcaCboICXSPR(cpuInfo, pJsonChild);

    return ret;
}

static const SUncoreMcaLogVx sUncoreMcaLogVx[] = {
    {cd_icx, logUncoreMcaICXSPR},
    {cd_icx2, logUncoreMcaICXSPR},
    {cd_spr, logUncoreMcaICXSPR},
    {cd_icxd, logUncoreMcaICXSPR},
};

/******************************************************************************
 *
 *   logUncoreMca
 *
 *   This function gathers the Uncore MCA register contents and adds them to the
 *   debug log.
 *
 ******************************************************************************/
int logUncoreMca(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    for (uint32_t i = 0;
         i < (sizeof(sUncoreMcaLogVx) / sizeof(SUncoreMcaLogVx)); i++)
    {
        if (cpuInfo->model == sUncoreMcaLogVx[i].cpuModel)
        {
            return sUncoreMcaLogVx[i].logUncoreMcaVx(cpuInfo, pJsonChild);
        }
    }

    CRASHDUMP_PRINT(ERR, stderr, "Cannot find version for %s\n", __FUNCTION__);
    return ACD_FAILURE;
}
