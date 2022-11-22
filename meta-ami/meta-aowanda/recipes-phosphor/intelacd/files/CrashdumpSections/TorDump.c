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

#include "TorDump.h"

#include "utils.h"

/******************************************************************************
 *
 *   torDumpJsonICX1
 *
 *   This function formats the TOR dump into a JSON object
 *
 ******************************************************************************/
static void torDumpJsonICXSPR(uint32_t u32Cha, uint32_t u32TorIndex,
                              uint32_t u32TorSubIndex, uint32_t u32PayloadBytes,
                              uint8_t* pu8TorCrashdumpData, cJSON* pJsonChild,
                              bool bInvalid, uint8_t cc, int ret)
{
    cJSON* channel;
    cJSON* tor;
    char jsonItemName[TD_JSON_STRING_LEN];
    char jsonItemString[TD_JSON_STRING_LEN];
    char jsonErrorString[TD_JSON_STRING_LEN];

    // Add the channel number item to the TOR dump JSON structure only if it
    // doesn't already exist
    cd_snprintf_s(jsonItemName, TD_JSON_STRING_LEN, TD_JSON_CHA_NAME, u32Cha);
    if ((channel = cJSON_GetObjectItemCaseSensitive(pJsonChild,
                                                    jsonItemName)) == NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              channel = cJSON_CreateObject());
    }

    // Add the TOR Index item to the TOR dump JSON structure only if it
    // doesn't already exist
    cd_snprintf_s(jsonItemName, TD_JSON_STRING_LEN, TD_JSON_TOR_NAME,
                  u32TorIndex);
    if ((tor = cJSON_GetObjectItemCaseSensitive(channel, jsonItemName)) == NULL)
    {
        cJSON_AddItemToObject(channel, jsonItemName,
                              tor = cJSON_CreateObject());
    }
    // Add the SubIndex data to the TOR dump JSON structure
    cd_snprintf_s(jsonItemName, TD_JSON_STRING_LEN, TD_JSON_SUBINDEX_NAME,
                  u32TorSubIndex);
    if (bInvalid)
    {
        cd_snprintf_s(jsonItemString, TD_JSON_STRING_LEN, TD_FIXED_DATA_CC_RC,
                      cc, ret);
        cJSON_AddStringToObject(tor, jsonItemName, jsonItemString);
        return;
    }
    else
    {
        cd_snprintf_s(jsonItemString, sizeof(jsonItemString), "0x0");
        bool leading = true;
        char* ptr = &jsonItemString[2];

        for (int i = u32PayloadBytes - 1; i >= 0; i--)
        {
            // exclude any leading zeros per schema
            if (leading && pu8TorCrashdumpData[i] == 0)
            {
                continue;
            }
            leading = false;

            ptr += cd_snprintf_s(ptr, u32PayloadBytes, "%02x",
                                 pu8TorCrashdumpData[i]);
        }
        if (PECI_CC_UA(cc))
        {
            cd_snprintf_s(jsonErrorString, TD_JSON_STRING_LEN, TD_DATA_CC_RC,
                          cc, ret);
            strcat_s(jsonItemString, TD_JSON_STRING_LEN, jsonErrorString);
            cJSON_AddStringToObject(tor, jsonItemName, jsonItemString);
            return;
        }
    }
    cJSON_AddStringToObject(tor, jsonItemName, jsonItemString);
}

/******************************************************************************
 *
 *    logTorDumpICX1
 *
 *    BMC performs the TOR dump retrieve from the processor directly via
 *    PECI interface after a platform three (3) strike failure.
 *
 ******************************************************************************/
int logTorDumpICX1(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    (void)cpuInfo;
    (void)pJsonChild;
    // Not supported in A0
    return ACD_SUCCESS;
}

/******************************************************************************
 *
 *    logTorDumpICXSPR
 *
 *    BMC performs the TOR dump retrieve from the processor directly via
 *    PECI interface after a platform three (3) strike failure.
 *
 ******************************************************************************/
int logTorDumpICXSPR(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    uint8_t u8CrashdumpEnabled = 1;
    uint16_t u16CrashdumpNumAgents;
    uint64_t u64UniqueId;
    uint64_t u64PayloadExp;
    int ret = 0;
    uint8_t cc = 0;

    // Crashdump Discovery
    // Crashdump Enabled
    ret = peci_CrashDump_Discovery(cpuInfo->clientAddr, PECI_CRASHDUMP_ENABLED,
                                   0, 0, 0, sizeof(uint8_t),
                                   &u8CrashdumpEnabled, &cc);
    if ((ret != PECI_CC_SUCCESS) || (u8CrashdumpEnabled != 0))
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Tor Crashdump is disabled (%d) during discovery "
                        "(disabled:%d)\n",
                        ret, u8CrashdumpEnabled);
        return ret;
    }

    // Crashdump Number of Agents
    ret = peci_CrashDump_Discovery(
        cpuInfo->clientAddr, PECI_CRASHDUMP_NUM_AGENTS, 0, 0, 0,
        sizeof(uint16_t), (uint8_t*)&u16CrashdumpNumAgents, &cc);
    if (ret != PECI_CC_SUCCESS || u16CrashdumpNumAgents <= PECI_CRASHDUMP_TOR)
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Error (%d) during discovery (num of agents:%d)\n", ret,
                        u16CrashdumpNumAgents);
        return ret;
    }

    // Crashdump Agent Data
    // Agent Unique ID
    ret = peci_CrashDump_Discovery(
        cpuInfo->clientAddr, PECI_CRASHDUMP_AGENT_DATA, PECI_CRASHDUMP_AGENT_ID,
        PECI_CRASHDUMP_TOR, 0, sizeof(uint64_t), (uint8_t*)&u64UniqueId, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Error (%d) during discovery (id:0x%" PRIx64 ")\n", ret,
                        u64UniqueId);
        return ret;
    }

    // Agent Payload Size
    ret =
        peci_CrashDump_Discovery(cpuInfo->clientAddr, PECI_CRASHDUMP_AGENT_DATA,
                                 PECI_CRASHDUMP_AGENT_PARAM, PECI_CRASHDUMP_TOR,
                                 PECI_CRASHDUMP_PAYLOAD_SIZE, sizeof(uint64_t),
                                 (uint8_t*)&u64PayloadExp, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Error (%d) during discovery (payload:0x%" PRIx64 ")\n",
                        ret, u64PayloadExp);
        return ret;
    }

    uint32_t u32PayloadBytes = 1 << u64PayloadExp;

    // Crashdump Get Frames
    for (size_t cha = 0; cha < cpuInfo->chaCount; cha++)
    {
        for (uint32_t u32TorIndex = 0; u32TorIndex < TD_TORS_PER_CHA_ICX1;
             u32TorIndex++)
        {
            for (uint32_t u32TorSubIndex = 0;
                 u32TorSubIndex < TD_SUBINDEX_PER_TOR_ICX1; u32TorSubIndex++)
            {
                uint8_t* pu8TorCrashdumpData =
                    (uint8_t*)(calloc(u32PayloadBytes, sizeof(uint8_t)));
                bool bInvalid = false;
                if (pu8TorCrashdumpData == NULL)
                {
                    CRASHDUMP_PRINT(ERR, stderr,
                                    "Error allocating memory (size:%d)\n",
                                    u32PayloadBytes);
                    return ACD_ALLOCATE_FAILURE;
                }
                ret = peci_CrashDump_GetFrame(
                    cpuInfo->clientAddr, PECI_CRASHDUMP_TOR, cha,
                    (u32TorIndex | (u32TorSubIndex << 8)), u32PayloadBytes,
                    pu8TorCrashdumpData, &cc);

                if (ret != PECI_CC_SUCCESS)
                {
                    bInvalid = true;
                    CRASHDUMP_PRINT(ERR, stderr,
                                    "Error (%d) during GetFrame"
                                    "(cha:%d index:%d sub-index:%d)\n",
                                    ret, (int)cha, u32TorIndex, u32TorSubIndex);
                }
                torDumpJsonICXSPR(cha, u32TorIndex, u32TorSubIndex,
                                  u32PayloadBytes, pu8TorCrashdumpData,
                                  pJsonChild, bInvalid, cc, ret);

                free(pu8TorCrashdumpData);
            }
        }
    }

    return ret;
}

static const STorDumpVx sTorDumpVx[] = {
    {cd_icx, logTorDumpICX1},
    {cd_icx2, logTorDumpICXSPR},
    {cd_icxd, logTorDumpICXSPR},
    {cd_spr, logTorDumpICXSPR},
};

/******************************************************************************
 *
 *    logTorDump
 *
 *    BMC performs the TOR dump retrieve from the processor directly via
 *    PECI interface after a platform three (3) strike failure.
 *
 ******************************************************************************/
int logTorDump(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    for (uint32_t i = 0; i < (sizeof(sTorDumpVx) / sizeof(STorDumpVx)); i++)
    {
        if (cpuInfo->model == sTorDumpVx[i].cpuModel)
        {
            return sTorDumpVx[i].logTorDumpVx(cpuInfo, pJsonChild);
        }
    }

    CRASHDUMP_PRINT(ERR, stderr, "Cannot find version for %s\n", __FUNCTION__);
    return ACD_FAILURE;
}
