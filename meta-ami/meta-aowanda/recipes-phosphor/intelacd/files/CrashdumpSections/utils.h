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

#ifndef UTILS_H
#define UTILS_H

#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stdint.h>

#include "crashdump.h"

#define DEFAULT_INPUT_FILE "/usr/share/crashdump/input/crashdump_input_%s.json"
#define OVERRIDE_INPUT_FILE "/tmp/crashdump/input/crashdump_input_%s.json"
#define DEFAULT_TELEMETRY_FILE                                                 \
    "/usr/share/crashdump/input/telemetry_input_%s.json"
#define OVERRIDE_TELEMETRY_FILE "/tmp/crashdump/input/telemetry_input_%s.json"

#define PECI_CC_SKIP_CORE(cc)                                                  \
    (((cc) == PECI_DEV_CC_CATASTROPHIC_MCA_ERROR ||                            \
      (cc) == PECI_DEV_CC_NEED_RETRY || (cc) == PECI_DEV_CC_OUT_OF_RESOURCE || \
      (cc) == PECI_DEV_CC_UNAVAIL_RESOURCE ||                                  \
      (cc) == PECI_DEV_CC_INVALID_REQ || (cc) == PECI_DEV_CC_MCA_ERROR)        \
         ? true                                                                \
         : false)

#define PECI_CC_SKIP_SOCKET(cc)                                                \
    (((cc) == PECI_DEV_CC_PARITY_ERROR_ON_GPSB_OR_PMSB ||                      \
      (cc) == PECI_DEV_CC_PARITY_ERROR_ON_GPSB_OR_PMSB_IERR ||                 \
      (cc) == PECI_DEV_CC_PARITY_ERROR_ON_GPSB_OR_PMSB_MCA)                    \
         ? true                                                                \
         : false)

#define PECI_CC_UA(cc)                                                         \
    (((cc) != PECI_DEV_CC_SUCCESS && (cc) != PECI_DEV_CC_FATAL_MCA_DETECTED)   \
         ? true                                                                \
         : false)

#define FREE(ptr)                                                              \
    do                                                                         \
    {                                                                          \
        free(ptr);                                                             \
        ptr = NULL;                                                            \
    } while (0)

#define SET_BIT(val, pos) ((val) |= ((uint64_t)1 << ((uint64_t)pos)))
#define CLEAR_BIT(val, pos) ((val) &= ~((uint64_t)1 << ((uint64_t)pos)))
#define CHECK_BIT(val, pos) ((val) & ((uint64_t)1 << ((uint64_t)pos)))
#define CPU_STR_LEN 5
#define NAME_STR_LEN 255
#define DEFAULT_VALUE -1

void setFields(uint32_t* value, uint32_t msb, uint32_t lsb, uint32_t inputVal);
uint32_t getFields(uint32_t value, uint32_t msb, uint32_t lsb);
uint32_t bitField(uint32_t offset, uint32_t size, uint32_t val);

// crashdump_input_xxx.json #define(s) and c-helper functions
#define RECORD_ENABLE "_record_enable"
#define ENABLE 0

void updateRecordEnable(cJSON* root, bool enable);
cJSON* getCrashDataSection(cJSON* root, char* section, bool* enable);
cJSON* getCrashDataSectionRegList(cJSON* root, char* section, char* regType,
                                  bool* enable);
int getCrashDataSectionVersion(cJSON* root, char* section);
cJSON* readInputFile(const char* filename);
int cd_snprintf_s(char* str, size_t len, const char* format, ...);
cJSON* getCrashDataSectionBigCoreRegList(cJSON* root, char* version);
bool isBigCoreRegVersionMatch(cJSON* root, uint32_t version);
uint32_t getCrashDataSectionBigCoreSize(cJSON* root, char* version);
void storeCrashDataSectionBigCoreSize(cJSON* root, char* version,
                                      uint32_t totalSize);
cJSON* getCrashDataSectionObject(cJSON* root, char* section, char* firstLevel,
                                 char* secondLevel, bool* enable);

cJSON* selectAndReadInputFile(Model cpuModel, char** filename,
                              bool isTelemetry);
cJSON* getCrashDataSectionAddressMapRegList(cJSON* root);
cJSON* getCrashDataSectionObjectOneLevel(cJSON* root, char* section,
                                         const char* firstLevel, bool* enable);
void addDelayToSection(cJSON* cpu, CPUInfo* cpuInfo, char* sectionName,
                       struct timespec* crashdumpStart);
uint64_t tsToNanosecond(struct timespec* ts);
#endif // UTILS_H
