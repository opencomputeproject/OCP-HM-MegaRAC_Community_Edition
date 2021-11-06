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

#ifndef CRASHDUMP_H
#define CRASHDUMP_H

#include <cjson/cJSON.h>
#include <linux/peci-ioctl.h>
#include <peci.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "safe_str_lib.h"

#define CRASHDUMP_PRINT(level, fmt, ...) fprintf(fmt, __VA_ARGS__)
#define CRASHDUMP_VALUE_LEN 6
#define RESET_DETECTED_NAME "cpu%d.%s"

#define SPR_MODEL 0x000806F0
#define STEPPING_ICX 0
#define STEPPING_ICX2 2
#define STEPPING_SPR 0

#define RECORD_TYPE_OFFSET 24
#define RECORD_TYPE_CORECRASHLOG 0x04
#define RECORD_TYPE_UNCORESTATUSLOG 0x08
#define RECORD_TYPE_TORDUMP 0x09
#define RECORD_TYPE_METADATA 0x0B
#define RECORD_TYPE_PMINFO 0x0C
#define RECORD_TYPE_ADDRESSMAP 0x0D
#define RECORD_TYPE_BMCAUTONOMOUS 0x23
#define RECORD_TYPE_MCALOG 0x3E

#define PRODUCT_TYPE_OFFSET 12
#define PRODUCT_TYPE_BMCAUTONOMOUS 0x23
#define PRODUCT_TYPE_ICXSP 0x1A
#define PRODUCT_TYPE_ICXDSP 0x1B
#define PRODUCT_TYPE_SPR 0x1C

#define REVISION_OFFSET 0
#define REVISION_1 0x01

typedef enum
{
    EMERG,
    ALERT,
    CRIT,
    ERR,
    WARNING,
    NOTICE,
    INFO,
    DEBUG,
} severity;

typedef enum
{
    ACD_SUCCESS,
    ACD_FAILURE,
    ACD_INVALID_OBJECT,
    ACD_ALLOCATE_FAILURE
} acdStatus;

typedef enum
{
    STARTUP = 1,
    EVENT = 0,
    INVALID = 2,
    OVERWRITTEN = 3,
} cpuidState;

typedef enum
{
    ON = 1,
    OFF = 0,
    UNKNOWN = -1,
} pwState;

typedef enum
{
    BIG_CORE,
    MCA,
    UNCORE,
    TOR,
    PM_INFO,
    ADDRESS_MAP,
    METADATA,
    NUMBER_OF_SECTIONS,
} Section;

typedef enum
{
    cd_icx,
    cd_icx2,
    cd_spr,
    cd_icxd,
    cd_numberOfModels,
} Model;

typedef struct
{
    char* name;
    Section section;
    int record_type;
} CrashdumpSection;

typedef struct
{
    char* filenamePtr;
    cJSON* bufferPtr;
} JSONInfo;

typedef struct
{
    uint8_t coreMaskCc;
    int coreMaskRet;
    bool coreMaskValid;
    cpuidState source;
} COREMaskRead;

typedef struct
{
    uint8_t chaCountCc;
    int chaCountRet;
    bool chaCountValid;
    cpuidState source;
} CHACountRead;

typedef struct
{
    uint8_t cpuidCc;
    int cpuidRet;
    CPUModel cpuModel;
    uint8_t stepping;
    bool cpuidValid;
    cpuidState source;
} CPUIDRead;

typedef struct
{
    bool resetDetected;
    int resetCpu;
    int resetSection;
    int currentSection;
    int currentCpu;
} PlatformState;

typedef struct
{
    int clientAddr;
    Model model;
    uint64_t coreMask;
    uint64_t crashedCoreMask;
    uint8_t sectionMask;
    size_t chaCount;
    pwState initialPeciWake;
    JSONInfo inputFile;
    CPUIDRead cpuidRead;
    CHACountRead chaCountRead;
    COREMaskRead coreMaskRead;
    struct timespec launchDelay;
} CPUInfo;

typedef struct
{
    bool unique;
    char* filenames[cd_numberOfModels];
    cJSON* buffers[cd_numberOfModels];
} InputFileInfo;

typedef struct
{
    Model cpuModel;
    int data;
} VersionInfo;

extern const CrashdumpSection sectionNames[NUMBER_OF_SECTIONS];
extern int revision_uncore;
extern bool commonMetaDataEnabled;

void logCrashdumpVersion(cJSON* parent, CPUInfo* cpuInfo, int recordType);

#endif // CRASHDUMP_H
