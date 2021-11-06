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

#include "BigCore.h"

#include "utils.h"

#ifdef COMPILE_UNIT_TESTS
#define static
#endif

/******************************************************************************
 *
 *  readBigCoreInputRegs
 *
 ******************************************************************************/
static int readBigCoreInputRegs(SCrashdumpRegICX1* reg, cJSON* itItem)
{
    int position = 0;
    cJSON* itParams = NULL;

    cJSON_ArrayForEach(itParams, itItem)
    {
        switch (position)
        {
            case bigCoreRegName:
                reg->name = itParams->valuestring;
                break;
            case bigCoreRegSize:
                reg->size = itParams->valueint;
                break;
            default:
                break;
        }
        position++;
    }

    return 0;
}

/******************************************************************************
 *
 *  getTotalInputRegsSize
 *
 ******************************************************************************/
static uint32_t getTotalInputRegsSize(CPUInfo* cpuInfo, uint32_t version)
{
    char jsonItemName[NAME_STR_LEN] = {0};
    cd_snprintf_s(jsonItemName, NAME_STR_LEN, "0x%x", version);

    uint32_t size = getCrashDataSectionBigCoreSize(cpuInfo->inputFile.bufferPtr,
                                                   jsonItemName);

    // As an optimization, use the size calculated previously for this version
    // of records
    if (size != 0)
    {
        return size;
    }

    cJSON* regList = getCrashDataSectionBigCoreRegList(
        cpuInfo->inputFile.bufferPtr, jsonItemName);

    if (regList == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    cJSON* itItem = NULL;
    cJSON* itParams = NULL;

    uint32_t totalSize = 0;
    SCrashdumpRegICX1 reg = {0};

    cJSON_ArrayForEach(itItem, regList)
    {
        int position = 0;
        cJSON_ArrayForEach(itParams, itItem)
        {
            if (bigCoreRegSize == position)
            {
                reg.size = itParams->valueint;
                totalSize += reg.size;
            }
            position++;
        }
    }

    // Store size for future optimized retrival
    storeCrashDataSectionBigCoreSize(cpuInfo->inputFile.bufferPtr, jsonItemName,
                                     totalSize);

    return totalSize;
}

/******************************************************************************
 *
 *  getJsonDataString
 *
 *  This function takes the number of requested bytes at the index of the given
 *  data and writes it to the given string.  It returns the number of bytes
 *  written.
 *
 ******************************************************************************/
static int getJsonDataString(uint8_t* u8Data, uint32_t u32DataSize,
                             uint32_t u32DataIndex, uint32_t u32NumBytes,
                             char* pDataString, uint32_t u32StringSize)
{
    // check that we have enough data
    if (u32DataIndex + u32NumBytes > u32DataSize)
    {
        return -1;
    }
    // check that our string buffer is large enough
    if (u32NumBytes * 2 + SIZE_OF_0x0 > u32StringSize)
    {
        cd_snprintf_s(pDataString, u32StringSize, "String buffer too small");
        return u32NumBytes;
    }
    // initialize the string to "0"
    cd_snprintf_s(pDataString, u32StringSize, "0x0");
    // handle leading zeros
    bool leading = true;
    char* ptr = &pDataString[2];
    for (int i = u32NumBytes - 1; i >= 0; i--)
    {
        // exclude any leading zeros per the schema
        if (leading && u8Data[u32DataIndex + i] == 0)
        {
            continue;
        }
        leading = false;
        ptr += cd_snprintf_s(ptr, (pDataString + u32StringSize) - ptr, "%02x",
                             u8Data[u32DataIndex + i]);
    }
    return u32NumBytes;
}

static void logSqData(cJSON* sqDump, cJSON* thread, uint32_t u32CrashSize,
                      uint8_t* pu8Crashdump, uint32_t* u32DataIndex,
                      uint32_t u32NumReads, uint8_t cc, int retval,
                      uint32_t index)
{
    char jsonItemName[CD_JSON_STRING_LEN] = {0};
    char jsonItemString[CD_JSON_STRING_LEN] = {0};
    char jsonErrorString[CD_JSON_STRING_LEN] = {0};

    int ret = getJsonDataString(pu8Crashdump, u32CrashSize, *u32DataIndex,
                                CD_JSON_SQ_ENTRY_SIZE, jsonItemString,
                                sizeof(jsonItemString));
    if (ret < 0)
    {
        return;
    }
    *u32DataIndex += ret;
    cd_snprintf_s(jsonItemName, CD_JSON_STRING_LEN, CD_JSON_SQ_ENTRY_NAME,
                  index);
    if (u32NumReads < *u32DataIndex)
    {
        if (retval != PECI_CC_SUCCESS)
        {
            cd_snprintf_s(jsonItemString, CD_JSON_STRING_LEN,
                          CD_JSON_FIXED_DATA_CC_RC, cc, retval);
            cJSON_AddStringToObject(sqDump, jsonItemName, jsonItemString);
            return;
        }
        else if (PECI_CC_UA(cc))
        {
            cd_snprintf_s(jsonErrorString, CD_JSON_STRING_LEN,
                          CD_JSON_DATA_CC_RC, cc, retval);
            strcat_s(jsonItemString, CD_JSON_STRING_LEN, jsonErrorString);
            cJSON_AddStringToObject(sqDump, jsonItemName, jsonItemString);
            return;
        }
    }
    cJSON_AddStringToObject(sqDump, jsonItemName, jsonItemString);
}

static void logSqDataRaw(cJSON* sqDump, uint32_t u32CrashSize,
                         uint8_t* pu8Crashdump, uint32_t* u32DataIndex,
                         uint32_t index)
{
    char jsonItemName[CD_JSON_STRING_LEN] = {0};
    char jsonItemString[CD_JSON_STRING_LEN] = {0};
    int ret = getJsonDataString(pu8Crashdump, u32CrashSize, *u32DataIndex,
                                CD_JSON_SQ_ENTRY_SIZE, jsonItemString,
                                sizeof(jsonItemString));
    if (ret < 0)
    {
        return;
    }

    *u32DataIndex += ret;
    cd_snprintf_s(jsonItemName, CD_JSON_STRING_LEN, "raw_0x%x", index);
    cJSON_AddStringToObject(sqDump, jsonItemName, jsonItemString);
}

/******************************************************************************
 *
 *  registerDump
 *
 *  This function formats the Crashdump's registers into a JSON object
 *
 ******************************************************************************/
static uint32_t registerDump(CPUInfo* cpuInfo, uint32_t u32CoreNum,
                             uint32_t u32ThreadNum, uint32_t u32CrashSize,
                             uint32_t u32NumReads, uint8_t* pu8Crashdump,
                             cJSON* pJsonChild, uint8_t cc, int retval,
                             uint32_t version, cJSON** ppcore, cJSON** ppthread)
{
    char jsonItemName[CD_JSON_STRING_LEN] = {0};
    char jsonItemString[CD_JSON_STRING_LEN] = {0};
    char jsonErrorString[CD_JSON_STRING_LEN] = {0};

    // Add the core number item to the Crashdump JSON structure only if it
    // doesn't already exist
    cd_snprintf_s(jsonItemName, CD_JSON_STRING_LEN, CD_JSON_CORE_NAME,
                  u32CoreNum);
    if ((*ppcore = cJSON_GetObjectItemCaseSensitive(pJsonChild,
                                                    jsonItemName)) == NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              *ppcore = cJSON_CreateObject());
    }

    // Add the thread number item to the Crashdump JSON structure
    cd_snprintf_s(jsonItemName, CD_JSON_STRING_LEN, CD_JSON_THREAD_NAME,
                  u32ThreadNum);
    cJSON_AddItemToObject(*ppcore, jsonItemName,
                          *ppthread = cJSON_CreateObject());

    cd_snprintf_s(jsonItemName, NAME_STR_LEN, "0x%x", version);
    cJSON* regList = getCrashDataSectionBigCoreRegList(
        cpuInfo->inputFile.bufferPtr, jsonItemName);

    if (regList == NULL)
    {
        return u32CrashSize;
    }

    // set up the data index
    uint32_t u32DataIndex = 0;

    cJSON* itItem = NULL;
    cJSON_ArrayForEach(itItem, regList)
    {
        SCrashdumpRegICX1 reg = {NULL, 0};
        readBigCoreInputRegs(&reg, itItem);

        int ret =
            getJsonDataString(pu8Crashdump, u32CrashSize, u32DataIndex,
                              reg.size, jsonItemString, sizeof(jsonItemString));
        if (ret < 0)
        {
            return u32CrashSize;
        }
        u32DataIndex += ret;
        if (u32NumReads < u32DataIndex)
        {
            if (retval != PECI_CC_SUCCESS)
            {
                cd_snprintf_s(jsonItemString, CD_JSON_STRING_LEN,
                              CD_JSON_FIXED_DATA_CC_RC, cc, retval);
                cJSON_AddStringToObject(*ppthread, reg.name, jsonItemString);
                return u32CrashSize;
            }
            else if (PECI_CC_UA(cc))
            {
                cd_snprintf_s(jsonErrorString, CD_JSON_STRING_LEN,
                              CD_JSON_DATA_CC_RC, cc, retval);
                strcat_s(jsonItemString, CD_JSON_STRING_LEN, jsonErrorString);
                cJSON_AddStringToObject(*ppthread, reg.name, jsonItemString);
                return u32CrashSize;
            }
        }
        cJSON_AddStringToObject(*ppthread, reg.name, jsonItemString);
    }

    return u32DataIndex;
}

static void crashdumpJsonICX(CPUInfo* cpuInfo, uint32_t u32CoreNum,
                             uint32_t u32ThreadNum, uint32_t u32CrashSize,
                             uint32_t u32NumReads, uint8_t* pu8Crashdump,
                             cJSON* pJsonChild, uint8_t cc, int retval,
                             uint32_t version)
{
    cJSON* core = NULL;
    cJSON* thread = NULL;

    // Convert from number of qwords to number of bytes
    // and add uCrashdumpVerSize.raw.
    u32NumReads = (u32NumReads * 8) + 8;

    // get the registers section
    uint32_t u32DataIndex = registerDump(
        cpuInfo, u32CoreNum, u32ThreadNum, u32CrashSize, u32NumReads,
        pu8Crashdump, pJsonChild, cc, retval, version, &core, &thread);

    // if the # of bytes processed is greater or equal to # of bytes
    // remaining then return
    if (u32DataIndex >= u32CrashSize)
    {
        return;
    }
    uint32_t sqDataSize = (u32CrashSize - u32DataIndex);

    if (CD_JSON_NUM_SQ_ENTRIES * 8 == sqDataSize)
    {
        // if there is still data, it's the SQ data, so dump it
        // Add the SQ Dump item to the Crashdump JSON structure
        cJSON* sqDump;
        cJSON_AddItemToObject(core, "SQ", sqDump = cJSON_CreateObject());
        // Add the SQ data
        for (uint32_t i = 0; i < sqDataSize; i++)
        {
            logSqData(sqDump, thread, u32CrashSize, pu8Crashdump, &u32DataIndex,
                      u32NumReads, cc, retval, i);
        }
    }
    else
    {
        // log raw since amount of data remaining does not match the expected
        // entries
        cJSON* sqDump;
        cJSON_AddItemToObject(core, "SQ", sqDump = cJSON_CreateObject());

        char jsonItemString[CD_JSON_STRING_LEN] = {0};
        cd_snprintf_s(jsonItemString, CD_JSON_STRING_LEN, "0x%x", sqDataSize);
        cJSON_AddStringToObject(sqDump, "_error_unexpected_size",
                                jsonItemString);
        // Add the SQ data
        for (uint32_t i = 0; i < sqDataSize; i++)
        {
            logSqDataRaw(sqDump, u32CrashSize, pu8Crashdump, &u32DataIndex, i);
        }
    }
}

static void crashdumpJsonSPR(CPUInfo* cpuInfo, uint32_t u32CoreNum,
                             uint32_t u32ThreadNum, uint32_t u32CrashSize,
                             uint32_t u32NumReads, uint8_t* pu8Crashdump,
                             cJSON* pJsonChild, uint8_t cc, int retval,
                             uint32_t version)
{
    cJSON* core = NULL;
    cJSON* thread = NULL;

    // Convert from number of qwords to number of bytes
    // and add uCrashdumpVerSize.raw.
    u32NumReads = (u32NumReads * 8) + 8;

    // get the registers section
    uint32_t u32DataIndex = registerDump(
        cpuInfo, u32CoreNum, u32ThreadNum, u32CrashSize, u32NumReads,
        pu8Crashdump, pJsonChild, cc, retval, version, &core, &thread);

    // if the # of bytes processed is greater or equal to # of bytes
    // remaining then return
    if (u32DataIndex >= u32CrashSize)
    {
        return;
    }

    uint32_t sqDataSize = (u32CrashSize - u32DataIndex);
    if ((CD_JSON_NUM_SPR_SQD_ENTRIES + CD_JSON_NUM_SPR_SQS_ENTRIES) * 8 ==
        sqDataSize)
    {
        // if there is still data, it's the SQ data, so dump it
        // Add the SQ Dump item to the Crashdump JSON structure
        cJSON* sqDump;
        // Add the SQD data
        cJSON_AddItemToObject(core, "SQD", sqDump = cJSON_CreateObject());
        for (uint32_t i = 0; i < CD_JSON_NUM_SPR_SQD_ENTRIES; i++)
        {
            logSqData(sqDump, thread, u32CrashSize, pu8Crashdump, &u32DataIndex,
                      u32NumReads, cc, retval, i);
        }

        // Add the SQS data
        cJSON_AddItemToObject(core, "SQS", sqDump = cJSON_CreateObject());
        for (uint32_t i = 0; i < CD_JSON_NUM_SPR_SQS_ENTRIES; i++)
        {
            logSqData(sqDump, thread, u32CrashSize, pu8Crashdump, &u32DataIndex,
                      u32NumReads, cc, retval, i);
        }
    }
    else
    {
        // log raw since amount of data remaining does not match the expected
        // entries
        cJSON* sqDump;
        cJSON_AddItemToObject(core, "SQD", sqDump = cJSON_CreateObject());

        char jsonItemString[CD_JSON_STRING_LEN] = {0};
        cd_snprintf_s(jsonItemString, CD_JSON_STRING_LEN, "0x%x", sqDataSize);
        cJSON_AddStringToObject(sqDump, "_error_unexpected_size",
                                jsonItemString);
        // Add the SQ data
        for (uint32_t i = 0; i < sqDataSize; i++)
        {
            logSqDataRaw(sqDump, u32CrashSize, pu8Crashdump, &u32DataIndex, i);
        }
    }
}

/******************************************************************************
 *
 *  rawdumpJsonICXSPR
 *
 ******************************************************************************/

static void rawdumpJsonICXSPR(uint32_t u32CoreNum, uint32_t u32ThreadNum,
                              uint32_t u32CrashSize, uint8_t* pu8Crashdump,
                              cJSON* pJsonChild, uint32_t u32DataIndex)
{
    char jsonItemName[CD_JSON_STRING_LEN] = {0};
    char jsonItemString[CD_JSON_STRING_LEN] = {0};
    cJSON* core = NULL;
    cJSON* thread = NULL;
    uint32_t rawItem = 0;
    cd_snprintf_s(jsonItemName, CD_JSON_STRING_LEN, CD_JSON_CORE_NAME,
                  u32CoreNum);
    if ((core = cJSON_GetObjectItemCaseSensitive(pJsonChild, jsonItemName)) ==
        NULL)
    {
        cJSON_AddItemToObject(pJsonChild, jsonItemName,
                              core = cJSON_CreateObject());
    }
    cd_snprintf_s(jsonItemName, CD_JSON_STRING_LEN, CD_JSON_THREAD_NAME,
                  u32ThreadNum);
    if ((thread = cJSON_GetObjectItemCaseSensitive(core, jsonItemName)) == NULL)
    {
        cJSON_AddItemToObject(core, jsonItemName,
                              thread = cJSON_CreateObject());
    }

    // Add the thread number item to the Crashdump JSON structure
    while (u32DataIndex < u32CrashSize)
    {
        rawItem = u32DataIndex;
        int ret = getJsonDataString(pu8Crashdump, u32CrashSize, u32DataIndex, 8,
                                    jsonItemString, sizeof(jsonItemString));
        if (ret < 0)
        {
            break;
        }
        cd_snprintf_s(jsonItemName, CD_JSON_STRING_LEN, "raw_0x%x", rawItem);
        rawItem += 8;
        u32DataIndex += ret;
        cJSON_AddStringToObject(thread, jsonItemName, jsonItemString);
    }
}

static inline bool isStoreDecoded(CPUInfo* cpuInfo, uint32_t totalInputSize,
                                  UCrashdumpVerSize uCrashdumpVerSize,
                                  uint32_t u32version)
{
    return (
        isBigCoreRegVersionMatch(cpuInfo->inputFile.bufferPtr, u32version) &&
        totalInputSize == (uCrashdumpVerSize.field.regDumpSize));
}

static inline uint32_t
    calculateCrashdumpSize(CPUInfo* cpuInfo,
                           UCrashdumpVerSize uCrashdumpVerSize)
{
    if (cd_icx == cpuInfo->model)
    {
        return uCrashdumpVerSize.field.regDumpSize +
               uCrashdumpVerSize.field.sqDumpSize + ICX_A0_FRAME_BYTE_OFFSET;
    }

    return uCrashdumpVerSize.field.regDumpSize +
           uCrashdumpVerSize.field.sqDumpSize;
}

/******************************************************************************
 *
 *  logCrashdumpICXSPR
 *
 *  BMC performs the crashdump retrieve from the processor directly via
 *  PECI interface for internal state of the cores and Cbos after a platform
 *  three (3) strike failure. The Crash Dump from CPU will be empty (size 0)
 *  if no cores qualify to be dumped. A core will not be dumped if the power
 *  state is such that it cannot be accessed. A core will be dumped only if it
 *  has experienced a "3-strike" Machine Check Error.
 *
 ******************************************************************************/
int logCrashdumpICXSPR(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    int ret = 0;
    uint8_t cc = 0;
    bool gotoNextCore = false;
    uint32_t u32NumReads = 0;

    // Crashdump Discovery
    uint8_t u8CrashdumpDisabled = ICX_A0_CRASHDUMP_DISABLED;
    ret = peci_CrashDump_Discovery(cpuInfo->clientAddr, PECI_CRASHDUMP_ENABLED,
                                   0, 0, 0, sizeof(uint8_t),
                                   &u8CrashdumpDisabled, &cc);
    if (ret != PECI_CC_SUCCESS ||
        u8CrashdumpDisabled == ICX_A0_CRASHDUMP_DISABLED)
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Crashdump is disabled (%d) during discovery "
                        "(disabled:%d)\n",
                        ret, u8CrashdumpDisabled);
        return ret;
    }

    // Crashdump Number of Agents
    uint16_t u16CrashdumpNumAgents;
    ret = peci_CrashDump_Discovery(
        cpuInfo->clientAddr, PECI_CRASHDUMP_NUM_AGENTS, 0, 0, 0,
        sizeof(uint16_t), (uint8_t*)&u16CrashdumpNumAgents, &cc);
    if (ret != PECI_CC_SUCCESS || u16CrashdumpNumAgents <= PECI_CRASHDUMP_CORE)
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Error (%d) during discovery (num of agents:%d)\n", ret,
                        u16CrashdumpNumAgents);
        return ret;
    }

    // Crashdump Agent Data
    // Agent Unique ID
    uint64_t u64UniqueId;
    ret = peci_CrashDump_Discovery(
        cpuInfo->clientAddr, PECI_CRASHDUMP_AGENT_DATA, PECI_CRASHDUMP_AGENT_ID,
        PECI_CRASHDUMP_CORE, 0, sizeof(uint64_t), (uint8_t*)&u64UniqueId, &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Error (%d) during discovery (id:0x%" PRIx64 ")\n", ret,
                        u64UniqueId);
        return ret;
    }

    // Agent Payload Size
    uint64_t u64PayloadExp;
    ret = peci_CrashDump_Discovery(
        cpuInfo->clientAddr, PECI_CRASHDUMP_AGENT_DATA,
        PECI_CRASHDUMP_AGENT_PARAM, PECI_CRASHDUMP_CORE,
        PECI_CRASHDUMP_PAYLOAD_SIZE, sizeof(uint64_t), (uint8_t*)&u64PayloadExp,
        &cc);
    if (ret != PECI_CC_SUCCESS)
    {
        CRASHDUMP_PRINT(ERR, stderr,
                        "Error (%d) during discovery (payload:0x%" PRIx64 ")\n",
                        ret, u64PayloadExp);
        return ret;
    }

    // Crashdump Get Frames
    // Go through each enabled core
    for (uint32_t u32CoreNum = 0; (cpuInfo->coreMask >> u32CoreNum) != 0;
         u32CoreNum++)
    {
        if (!CHECK_BIT(cpuInfo->coreMask, u32CoreNum))
        {
            continue;
        }
        uint32_t u32Threads = CD_ST_THREADS_PER_CORE;
        for (uint32_t u32ThreadNum = 0; u32ThreadNum < u32Threads;
             u32ThreadNum++)
        {
            // Get the crashdump size for this thread from the first read
            UCrashdumpVerSize uCrashdumpVerSize;
            ret = peci_CrashDump_GetFrame(
                cpuInfo->clientAddr, PECI_CRASHDUMP_CORE, u32CoreNum, 0,
                sizeof(uint64_t), (uint8_t*)&uCrashdumpVerSize.raw, &cc);

            if (ret != PECI_CC_SUCCESS)
            {
                CRASHDUMP_PRINT(ERR, stderr,
                                "Error (%d) during GetFrame 0 (0x%" PRIx64
                                ")\n",
                                ret, uCrashdumpVerSize.raw);
                continue;
            }

            if (PECI_CC_SKIP_CORE(cc))
            {
                break;
            }
            if (PECI_CC_SKIP_SOCKET(cc))
            {
                return ret;
            }

            uint32_t u32CrashdumpSize = 0;
            u32CrashdumpSize =
                calculateCrashdumpSize(cpuInfo, uCrashdumpVerSize);

            uint32_t u32version = uCrashdumpVerSize.field.version;

            uint64_t* pu64Crashdump = (uint64_t*)(calloc(u32CrashdumpSize, 1));
            if (pu64Crashdump == NULL)
            {
                // calloc failed, exit
                CRASHDUMP_PRINT(ERR, stderr,
                                "Error allocating memory (size:%d)\n",
                                u32CrashdumpSize);
                return SIZE_FAILURE;
            }
            pu64Crashdump[0] = uCrashdumpVerSize.raw;

            // log crashed core
            SET_BIT(cpuInfo->crashedCoreMask, u32CoreNum);

            // Get the rest of the crashdump data
            for (uint32_t i = 1; i < (u32CrashdumpSize / sizeof(uint64_t)); i++)
            {
                ret = peci_CrashDump_GetFrame(
                    cpuInfo->clientAddr, PECI_CRASHDUMP_CORE, u32CoreNum, 0,
                    sizeof(uint64_t), (uint8_t*)&pu64Crashdump[i], &cc);
                u32NumReads = i;
                if (PECI_CC_SKIP_CORE(cc))
                {
                    CRASHDUMP_PRINT(ERR, stderr,
                                    "Error (%d) during GetFrame (num:%d)\n",
                                    ret, i);
                    gotoNextCore = true;
                    break;
                }

                if (PECI_CC_SKIP_SOCKET(cc))
                {
                    CRASHDUMP_PRINT(ERR, stderr,
                                    "Error (%d) during GetFrame (num:%d)\n",
                                    ret, i);
                    FREE(pu64Crashdump);
                    return ret;
                }

                if (ret != PECI_CC_SUCCESS)
                {
                    CRASHDUMP_PRINT(ERR, stderr,
                                    "Error (%d) during GetFrame (num:%d)\n",
                                    ret, i);
                    gotoNextCore = true;
                    break;
                }
            }
            // Check if this core is multi-threaded, if available
            if (u32CrashdumpSize >= (CD_WHO_MISC_OFFSET + 1) * sizeof(uint64_t))
            {
                UCrashdumpWhoMisc uCrashdumpWhoMisc;
                uCrashdumpWhoMisc.raw = pu64Crashdump[CD_WHO_MISC_OFFSET];
                if (uCrashdumpWhoMisc.field.multithread)
                {
                    u32Threads = CD_MT_THREADS_PER_CORE;
                }
            }

            uint8_t threadId = 0;
            if (CHECK_BIT(pu64Crashdump[CD_WHO_MISC_OFFSET], 0))
            {
                threadId = 1;
            }

            uint32_t totalInputSize = 0;
            totalInputSize = getTotalInputRegsSize(cpuInfo, u32version);

            // Log this Crashdump
            if (isStoreDecoded(cpuInfo, totalInputSize, uCrashdumpVerSize,
                               u32version))
            {
                switch (cpuInfo->model)
                {
                    case cd_icx:
                    case cd_icx2:
                    case cd_icxd:
                        crashdumpJsonICX(cpuInfo, u32CoreNum, threadId,
                                         u32CrashdumpSize, u32NumReads,
                                         (uint8_t*)pu64Crashdump, pJsonChild,
                                         cc, ret, u32version);
                        break;
                    case cd_spr:
                        crashdumpJsonSPR(cpuInfo, u32CoreNum, threadId,
                                         u32CrashdumpSize, u32NumReads,
                                         (uint8_t*)pu64Crashdump, pJsonChild,
                                         cc, ret, u32version);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                if ((u32CrashdumpSize > CRASHDUMP_MAX_SIZE))
                {
                    u32CrashdumpSize = CRASHDUMP_MAX_SIZE;
                }
                rawdumpJsonICXSPR(u32CoreNum, threadId, u32CrashdumpSize,
                                  (uint8_t*)pu64Crashdump, pJsonChild, 0);
            }

            FREE(pu64Crashdump);
            if (gotoNextCore == true)
                break;
        }
    }
    return ret;
}

static const SCrashdumpVx sCrashdumpVx[] = {
    {cd_icx, logCrashdumpICXSPR},
    {cd_icx2, logCrashdumpICXSPR},
    {cd_icxd, logCrashdumpICXSPR},
    {cd_spr, logCrashdumpICXSPR},
};

/******************************************************************************
 *
 *  logCrashdump
 *
 *  BMC performs the crashdump retrieve from the processor directly via
 *  PECI interface for internal state of the cores and Cbos after a platform
 *  three (3) strike failure. The Crash Dump from CPU will be empty (size 0)
 *  if no cores qualify to be dumped. A core will not be dumped if the power
 *  state is such that it cannot be accessed. A core will be dumped only if
 *  it has experienced a "3-strike" Machine Check Error.
 *
 ******************************************************************************/
int logCrashdump(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }

    for (uint32_t i = 0; i < (sizeof(sCrashdumpVx) / sizeof(SCrashdumpVx)); i++)
    {
        if (cpuInfo->model == sCrashdumpVx[i].cpuModel)
        {
            return sCrashdumpVx[i].logCrashdumpVx(cpuInfo, pJsonChild);
        }
    }

    CRASHDUMP_PRINT(ERR, stderr, "Cannot find version for %s\n", __FUNCTION__);
    return ACD_FAILURE;
}
