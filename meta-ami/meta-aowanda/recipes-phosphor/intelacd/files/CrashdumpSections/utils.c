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

#include "utils.h"

#include <safe_mem_lib.h>
#include <safe_str_lib.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int cd_snprintf_s(char* str, size_t len, const char* format, ...)
{
    int ret;
    va_list args;
    va_start(args, format);
    ret = vsnprintf_s(str, len, format, args);
    va_end(args);
    return ret;
}

static uint32_t setMask(uint32_t msb, uint32_t lsb)
{
    uint32_t range = (msb - lsb) + 1;
    uint32_t mask = ((1u << range) - 1);
    return mask;
}

void setFields(uint32_t* value, uint32_t msb, uint32_t lsb, uint32_t setVal)
{
    uint32_t mask = setMask(msb, lsb);
    setVal = setVal << lsb;
    *value &= ~(mask << lsb);
    *value |= setVal;
}

uint32_t getFields(uint32_t value, uint32_t msb, uint32_t lsb)
{
    uint32_t mask = setMask(msb, lsb);
    value = value >> lsb;
    return (mask & value);
}

uint32_t bitField(uint32_t offset, uint32_t size, uint32_t val)
{
    uint32_t mask = (1u << size) - 1;
    return (val & mask) << offset;
}

cJSON* readInputFile(const char* filename)
{
    char* buffer = NULL;
    cJSON* jsonBuf = NULL;
    long int length = 0;
    FILE* fp = fopen(filename, "r");
    size_t result = 0;

    if (fp == NULL)
    {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    if (length == -1L)
    {
        fclose(fp);
        return NULL;
    }
    fseek(fp, 0, SEEK_SET);
    buffer = (char*)calloc(length, sizeof(char));
    if (buffer)
    {
        result = fread(buffer, 1, length, fp);
        if ((int)result != length)
        {
            fprintf(stderr, "fread read %zu bytes, but length is %ld", result,
                    length);
            fclose(fp);
            FREE(buffer);
            return NULL;
        }
    }

    fclose(fp);
    // Convert and return cJSON object from buffer
    jsonBuf = cJSON_Parse(buffer);
    FREE(buffer);
    return jsonBuf;
}

cJSON* getCrashDataSection(cJSON* root, char* section, bool* enable)
{
    *enable = false;
    cJSON* child = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(root, "crash_data"), section);

    if (child != NULL)
    {
        cJSON* recordEnable = cJSON_GetObjectItem(child, RECORD_ENABLE);
        if (recordEnable == NULL)
        {
            *enable = true;
        }
        else
        {
            *enable = cJSON_IsTrue(recordEnable);
        }
    }

    return child;
}

cJSON* getCrashDataSectionRegList(cJSON* root, char* section, char* regType,
                                  bool* enable)
{
    cJSON* child = getCrashDataSection(root, section, enable);

    if (child != NULL)
    {
        return cJSON_GetObjectItemCaseSensitive(
            cJSON_GetObjectItemCaseSensitive(child, regType), "reg_list");
    }

    return child;
}

cJSON* getCrashDataSectionObjectOneLevel(cJSON* root, char* section,
                                         const char* firstLevel, bool* enable)
{
    cJSON* child = getCrashDataSection(root, section, enable);

    if (child != NULL)
    {
        return cJSON_GetObjectItemCaseSensitive(child, firstLevel);
    }

    return child;
}

cJSON* getCrashDataSectionObject(cJSON* root, char* section, char* firstLevel,
                                 char* secondLevel, bool* enable)
{
    cJSON* child = getCrashDataSection(root, section, enable);

    if (child != NULL)
    {
        return cJSON_GetObjectItemCaseSensitive(
            cJSON_GetObjectItemCaseSensitive(child, firstLevel), secondLevel);
    }

    return child;
}

int getCrashDataSectionVersion(cJSON* root, char* section)
{
    uint64_t version = 0;
    bool enable = false;
    cJSON* child = getCrashDataSection(root, section, &enable);

    if (child != NULL)
    {
        cJSON* jsonVer = cJSON_GetObjectItem(child, "_version");
        if ((jsonVer != NULL) && cJSON_IsString(jsonVer))
        {
            version = strtoull(jsonVer->valuestring, NULL, 16);
        }
    }

    return version;
}

void updateRecordEnable(cJSON* root, bool enable)
{
    cJSON* logSection = NULL;
    logSection = cJSON_GetObjectItemCaseSensitive(root, RECORD_ENABLE);
    if (logSection == NULL)
    {
        cJSON_AddBoolToObject(root, RECORD_ENABLE, enable);
    }
}

bool isBigCoreRegVersionMatch(cJSON* root, uint32_t version)
{
    bool enable = false;
    cJSON* child = getCrashDataSection(root, "big_core", &enable);

    if (child != NULL && enable)
    {
        char jsonItemName[NAME_STR_LEN] = {0};
        cd_snprintf_s(jsonItemName, NAME_STR_LEN, "0x%x", version);

        cJSON* decodeObject = cJSON_GetObjectItemCaseSensitive(child, "decode");

        if (decodeObject != NULL)
        {
            cJSON* versionObject =
                cJSON_GetObjectItem(decodeObject, jsonItemName);
            if ((versionObject != NULL) && cJSON_IsObject(versionObject))
            {
                return true;
            }
        }
    }

    return false;
}

cJSON* getCrashDataSectionBigCoreRegList(cJSON* root, char* version)
{
    bool enable = false;
    cJSON* child = getCrashDataSection(root, "big_core", &enable);

    if (child != NULL && enable)
    {
        cJSON* decodeObject = cJSON_GetObjectItemCaseSensitive(child, "decode");

        if (decodeObject != NULL)
        {
            cJSON* versionObject =
                cJSON_GetObjectItemCaseSensitive(decodeObject, version);
            if (versionObject != NULL)
            {
                return cJSON_GetObjectItemCaseSensitive(versionObject,
                                                        "reg_list");
            }
            return versionObject;
        }
        return decodeObject;
    }
    return child;
}

uint32_t getCrashDataSectionBigCoreSize(cJSON* root, char* version)
{
    uint32_t size = 0;
    bool enable = false;
    cJSON* child = getCrashDataSection(root, "big_core", &enable);

    if (child != NULL && enable)
    {
        cJSON* decodeObject = cJSON_GetObjectItemCaseSensitive(child, "decode");

        if (decodeObject != NULL)
        {
            cJSON* versionObject =
                cJSON_GetObjectItemCaseSensitive(decodeObject, version);

            if (versionObject != NULL)
            {
                cJSON* jsonSize =
                    cJSON_GetObjectItemCaseSensitive(versionObject, "_size");

                if ((jsonSize != NULL) && cJSON_IsString(jsonSize))
                {
                    size = strtoul(jsonSize->valuestring, NULL, 16);
                }
            }
        }
    }

    return size;
}

void storeCrashDataSectionBigCoreSize(cJSON* root, char* version,
                                      uint32_t totalSize)
{
    bool enable = false;
    cJSON* child = getCrashDataSection(root, "big_core", &enable);

    if (child != NULL && enable)
    {
        char jsonItemName[NAME_STR_LEN] = {0};
        cd_snprintf_s(jsonItemName, NAME_STR_LEN, "0x%x", totalSize);
        cJSON_AddStringToObject(
            cJSON_GetObjectItemCaseSensitive(
                cJSON_GetObjectItemCaseSensitive(child, "decode"), version),
            "_size", jsonItemName);
    }
}

cJSON* selectAndReadInputFile(Model cpuModel, char** filename, bool isTelemetry)
{
    char cpuStr[CPU_STR_LEN] = {0};
    char nameStr[NAME_STR_LEN] = {0};

    switch (cpuModel)
    {
        case cd_icx:
        case cd_icx2:
        case cd_icxd:
            strcpy_s(cpuStr, sizeof("icx"), "icx");
            break;
        case cd_spr:
            strcpy_s(cpuStr, sizeof("spr"), "spr");
            break;
        default:
            CRASHDUMP_PRINT(ERR, stderr,
                            "Error selecting input file (CPUID 0x%x).\n",
                            cpuModel);
            return NULL;
    }

    char* override_file = OVERRIDE_INPUT_FILE;

    if (isTelemetry)
    {
        override_file = OVERRIDE_TELEMETRY_FILE;
    }

    cd_snprintf_s(nameStr, NAME_STR_LEN, override_file, cpuStr);

    if (access(nameStr, F_OK) != -1)
    {
        CRASHDUMP_PRINT(INFO, stderr, "Using override file - %s\n", nameStr);
    }
    else
    {
        char* default_file = DEFAULT_INPUT_FILE;
        if (isTelemetry)
        {
            default_file = DEFAULT_TELEMETRY_FILE;
        }
        cd_snprintf_s(nameStr, NAME_STR_LEN, default_file, cpuStr);
    }
    *filename = (char*)malloc(sizeof(nameStr));
    if (*filename == NULL)
    {
        return NULL;
    }
    strcpy_s(*filename, sizeof(nameStr), nameStr);

    return readInputFile(nameStr);
}

cJSON* getCrashDataSectionAddressMapRegList(cJSON* root)
{
    bool enable = false;
    cJSON* child = getCrashDataSection(root, "address_map", &enable);

    if (child != NULL && enable)
    {
        return cJSON_GetObjectItemCaseSensitive(child, "reg_list");
    }

    return child;
}

uint64_t tsToNanosecond(struct timespec* ts)
{
    return (ts->tv_sec * (uint64_t)1e9 + ts->tv_nsec);
}

static inline void logDelayTime(cJSON* parent, const char* sectionName,
                                struct timespec delay)
{
    // Create an empty JSON object for this section if it doesn't already
    // exist
    cJSON* logSectionJson;
    if ((logSectionJson =
             cJSON_GetObjectItemCaseSensitive(parent, sectionName) == NULL))
    {
        cJSON_AddItemToObject(parent, sectionName,
                              logSectionJson = cJSON_CreateObject());
    }

    char timeString[64];

    cd_snprintf_s(timeString, sizeof(timeString), "%.2fs",
                  (double)tsToNanosecond(&delay) / 1e9);

    CRASHDUMP_PRINT(INFO, stderr, "Inserted delay of %s in %s section!\n",
                    timeString, sectionName);
    cJSON_AddStringToObject(logSectionJson, "_inserted_delay_sec", timeString);
}

static inline struct timespec
    calculateDelay(struct timespec* crashdumpStart,
                   uint32_t delayTimeFromInputFileInSec)
{
    struct timespec current = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &current);

    uint64_t runTimeInNs =
        tsToNanosecond(&current) - tsToNanosecond(crashdumpStart);

    uint64_t delayTimeFromInputFileInNs =
        delayTimeFromInputFileInSec * (uint64_t)1e9;

    struct timespec delay = {0, 0};
    if (runTimeInNs < delayTimeFromInputFileInNs)
    {
        delay.tv_sec = (delayTimeFromInputFileInNs - runTimeInNs) / 1e9;
        delay.tv_nsec =
            (delayTimeFromInputFileInNs - runTimeInNs) % (uint64_t)1e9;

        return delay;
    }

    return delay;
}

static inline uint32_t getDelayFromInputFile(CPUInfo* cpuInfo,
                                             char* sectionName)
{
    bool enable = false;
    cJSON* inputDelayField = getCrashDataSectionObjectOneLevel(
        cpuInfo->inputFile.bufferPtr, sectionName, "_required_delay_sec",
        &enable);

    if (inputDelayField != NULL && enable)
    {
        return (uint32_t)inputDelayField->valueint;
    }

    return 0;
}

static inline bool doAddDelay(CPUInfo* cpuInfo, struct timespec* crashdumpStart,
                              uint32_t delayTimeFromInputFileInSec)
{
    if (0 == delayTimeFromInputFileInSec)
    {
        return false;
    }

    cpuInfo->launchDelay =
        calculateDelay(crashdumpStart, delayTimeFromInputFileInSec);

    if (0 == tsToNanosecond(&cpuInfo->launchDelay))
    {
        return false;
    }

    return true;
}

void addDelayToSection(cJSON* cpu, CPUInfo* cpuInfo, char* sectionName,
                       struct timespec* crashdumpStart)
{
    if (doAddDelay(cpuInfo, crashdumpStart,
                   getDelayFromInputFile(cpuInfo, sectionName)))
    {
        logDelayTime(cpu, sectionName, cpuInfo->launchDelay);

        nanosleep(&cpuInfo->launchDelay, NULL);
    }
}