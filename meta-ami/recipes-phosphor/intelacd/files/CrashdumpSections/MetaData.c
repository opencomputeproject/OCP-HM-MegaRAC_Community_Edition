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

#include "MetaData.h"

#include "utils.h"

/******************************************************************************
 *
 *   fillCPUID
 *
 *   This function reads and fills in the cpu_type JSON info
 *
 ******************************************************************************/
int fillCPUID(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    char jsonItemString[SI_JSON_STRING_LEN] = {0};
    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;
    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }

    if (cpuInfo->cpuidRead.cpuidRet != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_FIXED_DATA_CC_RC,
                      cpuInfo->cpuidRead.cpuidCc, cpuInfo->cpuidRead.cpuidRet);
    }
    else if (PECI_CC_UA(cpuInfo->cpuidRead.cpuidCc))
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_INT_DATA_CC_RC,
                      cpuInfo->cpuidRead.cpuModel | cpuInfo->cpuidRead.stepping,
                      cpuInfo->cpuidRead.cpuidCc);
    }
    else
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%x",
                      cpuInfo->cpuidRead.cpuModel |
                          cpuInfo->cpuidRead.stepping);
    }
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillChaCount
 *
 *   This function reads and fills in the Cha count
 *
 ******************************************************************************/
int fillChaCount(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    char jsonItemString[SI_JSON_STRING_LEN] = {0};
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;
    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }
    if (cpuInfo->chaCountRead.chaCountRet != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_FIXED_DATA_CC_RC,
                      cpuInfo->chaCountRead.chaCountCc,
                      cpuInfo->chaCountRead.chaCountRet);
    }
    else if (PECI_CC_UA(cpuInfo->chaCountRead.chaCountCc))
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_INT_DATA_CC_RC,
                      cpuInfo->chaCount, cpuInfo->chaCountRead.chaCountCc,
                      cpuInfo->chaCountRead.chaCountRet);
    }
    else
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%x",
                      cpuInfo->chaCount);
    }
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillCoreMask
 *
 *   This function reads and fills in the Core Mask
 *
 ******************************************************************************/
int fillCoreMask(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    int cpuNum = cpuInfo->clientAddr & 0xF;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    char jsonItemString[SI_JSON_STRING_LEN] = {0};
    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }
    if (cpuInfo->coreMaskRead.coreMaskRet != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_FIXED_DATA_CC_RC,
                      cpuInfo->coreMaskRead.coreMaskCc,
                      cpuInfo->coreMaskRead.coreMaskRet);
    }
    else if (PECI_CC_UA(cpuInfo->coreMaskRead.coreMaskCc))
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_64_DATA_CC_RC,
                      cpuInfo->coreMask, cpuInfo->coreMaskRead.coreMaskCc,
                      cpuInfo->coreMaskRead.coreMaskRet);
    }
    else
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%llx",
                      cpuInfo->coreMask);
    }
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillSource
 *
 *   This function various Sources for of each cpu
 *
 ******************************************************************************/
int fillSource(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild,
               cpuidState source)
{
    cJSON* cpu = NULL;
    int cpuNum = cpuInfo->clientAddr & 0xF;
    char jsonItemString[SI_JSON_STRING_LEN] = {0};
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    // For now, the CPU number is just the bottom nibble of the PECI client ID
    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }
    switch (source)
    {
        case STARTUP:
            cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_STARTUP);
            break;
        case EVENT:
            cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_EVENT);
            break;
        case OVERWRITTEN:
            cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_OVERWRITTEN);
            break;
        case INVALID:
            cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_INVALID);
            break;
        default:
            return false;
    }
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillCPUIdSource
 *
 *   This function reads and fills the CPUId Source of each cpu
 *
 ******************************************************************************/
int fillCPUIdSource(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    return fillSource(cpuInfo, cSectionName, pJsonChild,
                      cpuInfo->cpuidRead.source);
}

/******************************************************************************
 *
 *   fillCoreMaskSource
 *
 *   This function reads and fills the Core Mask Source of each cpu
 *
 ******************************************************************************/
int fillCoreMaskSource(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    return fillSource(cpuInfo, cSectionName, pJsonChild,
                      cpuInfo->coreMaskRead.source);
}

/******************************************************************************
 *
 *   fillChaCountSource
 *
 *   This function reads and fills the Cha Count Source of each cpu
 *
 ******************************************************************************/
int fillChaCountSource(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    return fillSource(cpuInfo, cSectionName, pJsonChild,
                      cpuInfo->chaCountRead.source);
}

/******************************************************************************
 *
 *   fillPECI
 *
 *   This function reads and fills in the cpu_stepping JSON info
 *
 ******************************************************************************/
int fillPECI(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    char jsonItemString[SI_JSON_STRING_LEN] = {0};

    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;

    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }

    cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%x",
                  cpuInfo->clientAddr);
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillPackageId
 *
 *   This function reads and fills in the package_id JSON info
 *
 ******************************************************************************/
int fillPackageId(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};

    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;

    // Add the socket number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }

    // Fill in package_id as "N/A" for now
    cJSON_AddStringToObject(cpu, cSectionName, MD_NA);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillCoresPerCpu
 *
 *   This function reads and fills in the cores_per_cpu JSON info
 *
 ******************************************************************************/
int fillCoresPerCpu(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    char jsonItemString[SI_JSON_STRING_LEN] = {0};

    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;

    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }
    if (cpuInfo->coreMaskRead.coreMaskRet != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_FIXED_DATA_CC_RC,
                      cpuInfo->coreMaskRead.coreMaskCc,
                      cpuInfo->coreMaskRead.coreMaskRet);
    }
    else if (PECI_CC_UA(cpuInfo->coreMaskRead.coreMaskCc))
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_INT_DATA_CC_RC,
                      __builtin_popcountll(cpuInfo->coreMask),
                      cpuInfo->coreMaskRead.coreMaskCc,
                      cpuInfo->coreMaskRead.coreMaskRet);
    }
    else
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%x",
                      __builtin_popcountll(cpuInfo->coreMask));
        cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    }
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillUCodeVersion
 *
 *   This function reads and fills in the ucode_patch_ver JSON info
 *
 ******************************************************************************/
int fillUCodeVersion(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    uint32_t u32PeciData = 0;
    uint8_t cc = 0;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    char jsonItemString[SI_JSON_STRING_LEN] = {0};
    int ret = 0;

    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;

    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }
    // Get the UCode Version
    ret = peci_RdPkgConfig(cpuInfo->clientAddr, PECI_MBX_INDEX_CPU_ID,
                           PECI_PKG_ID_MICROCODE_REV, sizeof(uint32_t),
                           (uint8_t*)&u32PeciData, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_FIXED_DATA_CC_RC,
                      cc, ret);
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_INT_DATA_CC_RC,
                      u32PeciData, cc, ret);
    }
    else
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%x", u32PeciData);
    }
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillVCodeVersion
 *
 *   This function reads and fills in the vcode_patch_ver JSON info
 *
 ******************************************************************************/
int fillVCodeVersion(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    uint32_t u32PeciData = 0;
    uint8_t cc = 0;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    char jsonItemString[SI_JSON_STRING_LEN] = {0};
    int ret = 0;

    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;

    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }
    // Get the VCode Version if available
    ret = peci_RdPkgConfig(cpuInfo->clientAddr, MBX_INDEX_VCU, VCU_VERSION,
                           sizeof(uint32_t), (uint8_t*)&u32PeciData, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_FIXED_DATA_CC_RC,
                      cc, ret);
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_INT_DATA_CC_RC,
                      u32PeciData, cc, ret);
    }
    else
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%x", u32PeciData);
    }
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ret;
}

/******************************************************************************
 *
 *   fillMeVersionJson
 *
 *   This function fills in the me_fw_ver JSON info
 *
 ******************************************************************************/
int fillMeVersion(char* cSectionName, cJSON* pJsonChild)
{
    // Fill in as N/A for now
    cJSON_AddStringToObject(pJsonChild, cSectionName, MD_NA);
    return ACD_SUCCESS;
    // char jsonItemString[SI_JSON_STRING_LEN];

    // cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN,
    // "%02d.%02x.%02x.%02x%x",
    // sSysInfoRawData->meFwDeviceId.u8FirmwareMajorVersion & 0x7f,
    // sSysInfoRawData->meFwDeviceId.u8B2AuxFwRevInfo & 0x0f,
    // sSysInfoRawData->meFwDeviceId.u8FirmwareMinorVersion,
    // sSysInfoRawData->meFwDeviceId.u8B1AuxFwRevInfo,
    // sSysInfoRawData->meFwDeviceId.u8B2AuxFwRevInfo >> 4);
    // cJSON_AddStringToObject(pJsonChild, cSectionName, jsonItemString);
}

/******************************************************************************
 *
 *   fillMcaErrSrcLog
 *
 *   This function fills in the mca_err_src_log JSON info
 *
 ******************************************************************************/
int fillMcaErrSrcLog(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    uint32_t u32PeciData = 0;
    uint8_t cc = 0;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    char jsonItemString[SI_JSON_STRING_LEN] = {0};
    int ret;

    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;

    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }
    ret = peci_RdPkgConfig(cpuInfo->clientAddr, PECI_MBX_INDEX_CPU_ID,
                           PECI_PKG_ID_MACHINE_CHECK_STATUS, sizeof(uint32_t),
                           (uint8_t*)&u32PeciData, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_FIXED_DATA_CC_RC,
                      cc, ret);
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_INT_DATA_CC_RC,
                      u32PeciData, cc, ret);
    }
    else
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%x", u32PeciData);
    }
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ret;
}

/******************************************************************************
 *
 *   fillCrashdumpVersion
 *
 *   This function fills in the crashdump_ver JSON info
 *
 ******************************************************************************/
int fillCrashdumpVersion(char* cSectionName, cJSON* pJsonChild)
{
    cJSON_AddStringToObject(pJsonChild, cSectionName, SI_CRASHDUMP_VER);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillPpinJson
 *
 *   This function fills in the PPIN JSON info
 *
 ******************************************************************************/
int getPpinDataICX1(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    uint32_t ppinUpper = 0;
    uint32_t ppinLower = 0;
    uint8_t cc = 0;
    uint64_t ppin = 0;
    int ret = 0;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    char jsonItemString[SI_JSON_STRING_LEN] = {0};

    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;

    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }

    ret = peci_RdPkgConfig(cpuInfo->clientAddr, SI_PECI_PPIN_IDX,
                           SI_PECI_PPIN_LOWER, sizeof(uint32_t),
                           (uint8_t*)&ppinLower, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_FIXED_DATA_CC_RC,
                      cc, ret);
        cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
        return ret;
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_INT_DATA_CC_RC,
                      ppinLower, cc, ret);
        cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
        return ret;
    }

    ret = peci_RdPkgConfig(cpuInfo->clientAddr, SI_PECI_PPIN_IDX,
                           SI_PECI_PPIN_UPPER, sizeof(uint32_t),
                           (uint8_t*)&ppinUpper, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_FIXED_DATA_CC_RC,
                      cc, ret);
        cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
        return ret;
    }
    else if (PECI_CC_UA(cc))
    {
        cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_INT_DATA_CC_RC,
                      ppinUpper, cc, ret);
        cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
        return ret;
    }
    ppin = ppinUpper;
    ppin <<= 32;
    ppin |= ppinLower;

    cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%" PRIx64 "", ppin);
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ACD_SUCCESS;
}

static const SPpinVx sSPpinVx[] = {
    {cd_icx, getPpinDataICX1},
    {cd_icx2, getPpinDataICX1},
    {cd_icxd, getPpinDataICX1},
    {cd_spr, getPpinDataICX1},
};

/******************************************************************************
 *
 *   fillInputFile
 *
 *   This function fills in the crashdump input filename.
 *
 ******************************************************************************/
int fillInputFile(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild,
                  InputFileInfo* inputFileInfo)
{
    cJSON* cpu = NULL;
    char jsonItemName[SI_JSON_STRING_LEN] = {0};

    if (inputFileInfo->unique)
    {
        cJSON_AddStringToObject(pJsonChild, cSectionName,
                                cpuInfo->inputFile.filenamePtr);
    }
    else
    {
        // For now, the CPU number is just the bottom nibble of the
        // PECI client ID
        int cpuNum = cpuInfo->clientAddr & 0xF;
        // Add the CPU number object if it doesn't already exist
        cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                      cpuNum);
        if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild,
                                                    jsonItemName)) == NULL)
        {
            cJSON_AddItemToObject(pJsonChild, jsonItemName,
                                  cpu = cJSON_CreateObject());
        }

        if (cpuInfo->inputFile.filenamePtr == NULL)
        {
            cJSON_AddStringToObject(cpu, cSectionName, MD_NA);
        }
        else
        {
            cJSON_AddStringToObject(cpu, cSectionName,
                                    cpuInfo->inputFile.filenamePtr);
        }
    }

    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillPpin
 *
 *   This function gets PPIN info
 *
 ******************************************************************************/
int fillPpin(CPUInfo* cpuInfo, char* cSectionName, cJSON* pJsonChild)
{
    int r = 0;
    int ret = 0;
    for (size_t i = 0; i < (sizeof(sSPpinVx) / sizeof(SPpinVx)); i++)
    {
        if (cpuInfo->model == sSPpinVx[i].cpuModel)
        {
            r = sSPpinVx[i].getPpinVx(cpuInfo, cSectionName, pJsonChild);
            if (r == 1)
            {
                ret = 1;
            }
        }
    }
    return ret;
}

/******************************************************************************
 *
 *   fillCrashCoreCount
 *
 *   This function gets the number of Crash Cores information
 *
 ******************************************************************************/
static int fillCrashCoreCount(CPUInfo* cpuInfo, char* cSectionName,
                              cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    char jsonItemString[SI_JSON_STRING_LEN] = {0};
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;
    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }
    // Get the crashCore data obtain during BigCore
    cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, "0x%x",
                  __builtin_popcountll(cpuInfo->crashedCoreMask));
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *   fillCrashCoreMask
 *
 *   This function gets the Crash Cores Mask information
 *
 ******************************************************************************/
static int fillCrashCoreMask(CPUInfo* cpuInfo, char* cSectionName,
                             cJSON* pJsonChild)
{
    cJSON* cpu = NULL;
    char jsonItemString[SI_JSON_STRING_LEN] = {0};
    char jsonItemName[SI_JSON_STRING_LEN] = {0};
    // For now, the CPU number is just the bottom nibble of the PECI client ID
    int cpuNum = cpuInfo->clientAddr & 0xF;
    // Add the CPU number object if it doesn't already exist
    cd_snprintf_s(jsonItemName, SI_JSON_STRING_LEN, SI_JSON_SOCKET_NAME,
                  cpuNum);
    if ((cpu = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              cpu = cJSON_CreateObject());
    }
    // Get the crashCore data obtain during BigCore
    cd_snprintf_s(jsonItemString, SI_JSON_STRING_LEN, MD_UINT64,
                  cpuInfo->crashedCoreMask);
    cJSON_AddStringToObject(cpu, cSectionName, jsonItemString);
    return ACD_SUCCESS;
}

static SSysInfoSection sSysInfoTable[] = {
    {"peci_id", fillPECI},
    {"cpuid", fillCPUID},
    {"_cpuid_source", fillCPUIdSource},
    {"_core_mask_source", fillCoreMaskSource},
    {"_cha_count_source", fillChaCountSource},
    {"core_mask", fillCoreMask},
    {"cha_count", fillChaCount},
    {"package_id", fillPackageId},
    {"core_count", fillCoresPerCpu},
    {"ucode_patch_ver", fillUCodeVersion},
    {"vcode_ver", fillVCodeVersion},
    {"mca_err_src_log", fillMcaErrSrcLog},
    {"ppin", fillPpin},
    {"crashcore_count", fillCrashCoreCount},
    {"crashcore_mask", fillCrashCoreMask},
};

static SSysInfoInputFileSection sSysInfoInputFileTable[] = {
    {"_input_file", fillInputFile},
};

/******************************************************************************
 *
 *   sysInfoJson
 *
 *   This function formats the system information into a JSON object
 *
 ******************************************************************************/
static int sysInfoJson(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    uint32_t i = 0;
    int r = 0;
    int ret = ACD_SUCCESS;

    for (i = 0; i < (sizeof(sSysInfoTable) / sizeof(SSysInfoSection)); i++)
    {
        r = sSysInfoTable[i].FillSysInfoJson(
            cpuInfo, sSysInfoTable[i].sectionName, pJsonChild);
        if (r == 1)
        {
            ret = ACD_FAILURE;
        }
    }
    return ret;
}

/******************************************************************************
 *
 *   logSysInfo
 *
 *   This function gathers various bits of system information and compiles them
 *   into a single group
 *
 ******************************************************************************/
int logSysInfo(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    commonMetaDataEnabled = CHECK_BIT(cpuInfo->sectionMask, METADATA);
    if (!commonMetaDataEnabled)
    {
        updateRecordEnable(pJsonChild, false);
        return ACD_SUCCESS;
    }

    // Log the System Info
    return sysInfoJson(cpuInfo, pJsonChild);
}

/******************************************************************************
 *
 *   logSysInfoInputFile
 *
 *   This function gathers various bits of input file information and compiles
 *   them into a single group
 *
 ******************************************************************************/
int logSysInfoInputfile(CPUInfo* cpuInfo, cJSON* pJsonChild,
                        InputFileInfo* inputFileInfo)
{
    uint32_t i = 0;
    int r = 0;
    int ret = 0;

    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    commonMetaDataEnabled = CHECK_BIT(cpuInfo->sectionMask, METADATA);
    if (!commonMetaDataEnabled)
    {
        commonMetaDataEnabled = false;
        updateRecordEnable(pJsonChild, false);
        return ACD_SUCCESS;
    }

    for (i = 0; i < (sizeof(sSysInfoInputFileTable) /
                     sizeof(SSysInfoInputFileSection));
         i++)
    {
        r = sSysInfoInputFileTable[i].FillSysInfoJson(
            cpuInfo, sSysInfoInputFileTable[i].sectionName, pJsonChild,
            inputFileInfo);
        if (r == 1)
        {
            ret = 1;
        }
    }
    return ret;
}

/******************************************************************************
 *
 *   logResetDetected
 *
 *   This function logs the section which was being captured when reset occured
 *
 ******************************************************************************/
int logResetDetected(cJSON* metadata, int cpuNum, int sectionName)
{
    char resetSection[SI_JSON_STRING_LEN];

    if (sectionName != DEFAULT_VALUE)
    {
        cd_snprintf_s(resetSection, SI_JSON_STRING_LEN, RESET_DETECTED_NAME,
                      cpuNum, sectionNames[sectionName].name);

        CRASHDUMP_PRINT(INFO, stderr, "Reset occured while in section %s\n",
                        resetSection);
    }
    else
    {
        cd_snprintf_s(resetSection, SI_JSON_STRING_LEN, "%s",
                      RESET_DETECTED_DEFAULT);
    }

    cJSON_AddStringToObject(metadata, "_reset_detected", resetSection);

    return 0;
}
