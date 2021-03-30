/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once
#include <ipmid/api.h>

#include <oemcommands.hpp>

constexpr uint8_t maxDirEntries = 4;
constexpr uint16_t msgPayloadSize = 1024 * 60;
constexpr uint32_t smbiosTableStorageSize = 64 * 1024;
constexpr uint32_t mdriiSMSize = 0x00100000;

struct DataIdStruct
{
    uint8_t dataInfo[16];
} __attribute__((packed));

struct Mdr2DirEntry
{
    DataIdStruct Id;
    uint32_t size;
    uint32_t dataSetSize;
    uint32_t dataVersion;
    uint32_t timestamp;
} __attribute__((packed));

// ====================== MDR II Pull Command Structures ======================
// MDR II Pull Agent status inquiry command
struct MDRiiGetAgentStatus
{
    uint16_t agentId;
    uint8_t dirVersion;
} __attribute__((packed));

// MDR II status inquiry response
struct MDRiiAgentStatusResponse
{
    uint8_t mdrVersion;
    uint8_t agentVersion;
    uint8_t dirVersion;
    uint8_t dirEntries;
    uint8_t dataRequest;
} __attribute__((packed));

// MDR II Pull Agent directory information inquiry command
struct MDRiiGetDirRequest
{
    uint16_t agentId;
    uint8_t dirIndex;
} __attribute__((packed));

// MDR II directory information inquiry response
struct MDRiiGetDirResponse
{
    uint8_t mdrVersion;
    uint8_t dirVersion;
    uint8_t returnedEntries;
    uint8_t remainingEntries;
    uint8_t data[1];
} __attribute__((packed));

// MDR II Pull Agent data set information inquiry command
struct MDRiiGetDataInfoRequest
{
    uint16_t agentId;
    DataIdStruct dataSetInfo;
} __attribute__((packed));

// MDR II data set information inquiry response
struct MDRiiGetDataInfoResponse
{
    uint8_t mdrVersion;
    DataIdStruct dataSetId;
    uint8_t validFlag;
    uint32_t dataLength;
    uint32_t dataVersion;
    uint32_t timeStamp;
} __attribute__((packed));

// MDR II Pull Agent lock data set command
struct MDRiiLockDataRequest
{
    uint16_t agentId;
    DataIdStruct dataSetInfo;
    uint16_t timeout;
} __attribute__((packed));

// MDR II Pull Agent lock data set response
struct MDRiiLockDataResponse
{
    uint8_t mdrVersion;
    uint16_t lockHandle;
    uint32_t dataLength;
    uint32_t xferAddress;
    uint32_t xferLength;
} __attribute__((packed));

// MDR II Pull Agent unlock data set command
struct MDRiiUnlockDataRequest
{
    uint16_t agentId;
    uint16_t lockHandle;
} __attribute__((packed));

// MDR II Pull Agent get data block command
struct MDRiiGetDataBlockRequest
{
    uint16_t agentId;
    uint16_t lockHandle;
    uint32_t xferOffset;
    uint32_t xferLength;
} __attribute__((packed));

// MDR II Pull Agent get data block response
struct MDRiiGetDataBlockResponse
{
    uint32_t xferLength;
    uint32_t checksum;
    uint8_t data[msgPayloadSize];
} __attribute__((packed));

// ====================== MDR II Push Command Structures ======================
// MDR II Push Agent send dir info command
struct MDRiiSendDirRequest
{
    uint16_t agentId;
    uint8_t dirVersion;
    uint8_t dirIndex;
    uint8_t returnedEntries;
    uint8_t remainingEntries;
    Mdr2DirEntry data[1]; // place holder for N directory entries
} __attribute__((packed));

// MDR II Push Agent offer data set info command
struct MDRiiOfferDataInfo
{
    uint16_t agentId;
} __attribute__((packed));

// MDR II Client send data set info offer response
struct MDRiiOfferDataInfoResponse
{
    DataIdStruct dataSetInfo;
} __attribute__((packed));

// MDR II Push Agent send data set info command
struct MDRiiSendDataInfoRequest
{
    uint16_t agentId;
    DataIdStruct dataSetInfo;
    uint8_t validFlag;
    uint32_t dataLength;
    uint32_t dataVersion; // Roughly equivalent to the "file name"
    uint32_t
        timeStamp; // More info on the identity of this particular set of data
} __attribute__((packed));

// MDR II Push Agent send data start command
struct MDRiiDataStartRequest
{
    uint16_t agentId;
    DataIdStruct dataSetInfo;
    uint32_t dataLength;
    uint32_t xferAddress;
    uint32_t xferLength;
    uint16_t timeout;
} __attribute__((packed));

// MDR II Client send data start response
struct MDRiiDataStartResponse
{
    uint8_t xferStartAck;
    uint16_t sessionHandle;
} __attribute__((packed));

// MDR II
struct MDRiiDataDoneRequest
{
    uint16_t agentId;
    uint16_t lockHandle;
} __attribute__((packed));

// MDR II Push Agent send data block command
struct MDRiiSendDataBlockRequest
{
    uint16_t agentId;
    uint16_t lockHandle;
    uint32_t xferOffset;
    uint32_t xferLength;
    uint32_t checksum;
} __attribute__((packed));
