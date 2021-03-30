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
#include "ipmid/api.h"

#include <oemcommands.hpp>

#include <cstddef>
#include <cstdint>

constexpr uint16_t msgPayloadSize = 1024 * 60;

typedef enum
{
    regionLockUnlocked = 0,
    regionLockStrict,
    regionLockPreemptable
} MDRLockType;

typedef struct
{
    uint8_t DirVer;
    uint8_t MDRType;
    uint16_t timestamp;
    uint16_t DataSize;
} __attribute__((packed)) MDRSmbios_Header;

typedef struct
{
    uint8_t MdrVersion;
    uint8_t regionId;
    bool valid;
    uint8_t updateCount;
    uint8_t lockPolicy;
    uint16_t regionLength;
    uint16_t regionUsed;
    uint8_t CRC8;
} __attribute__((packed)) MDRState;

struct RegionStatusRequest
{
    uint8_t regionId;
} __attribute__((packed));

struct RegionStatusResponse
{
    MDRState State;
} __attribute__((packed));

struct RegionCompleteRequest
{
    uint8_t sessionId;
    uint8_t regionId;
} __attribute__((packed));

struct RegionReadRequest
{
    uint8_t regionId;
    uint8_t length;
    uint16_t offset;
} __attribute__((packed));

struct RegionReadResponse
{
    uint8_t length;
    uint8_t updateCount;
    uint8_t data[msgPayloadSize];
} __attribute__((packed));

struct RegionWriteRequest
{
    uint8_t sessionId;
    uint8_t regionId;
    uint8_t length;
    uint16_t offset;
    uint8_t data[msgPayloadSize];
} __attribute__((packed));

struct RegionLockRequest
{
    uint8_t sessionId;
    uint8_t regionId;
    uint8_t lockPolicy;
    uint16_t msTimeout;
} __attribute__((packed));

constexpr size_t maxMDRId = 5;
