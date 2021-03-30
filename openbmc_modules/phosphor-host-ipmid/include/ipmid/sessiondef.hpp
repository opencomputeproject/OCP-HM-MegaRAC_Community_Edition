/*
 * Copyright © 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#pragma once

namespace session
{

static constexpr auto sessionManagerRootPath =
    "/xyz/openbmc_project/ipmi/session";
static constexpr auto sessionIntf = "xyz.openbmc_project.Ipmi.SessionInfo";
static constexpr uint8_t ipmi20VerSession = 0x01;
static constexpr size_t maxSessionCountPerChannel = 15;
static constexpr size_t sessionZero = 0;
static constexpr size_t maxSessionlessCount = 1;
static constexpr uint8_t invalidSessionID = 0;
static constexpr uint8_t invalidSessionHandle = 0;
static constexpr uint8_t defaultSessionHandle = 0xFF;
static constexpr uint8_t maxNetworkInstanceSupported = 4;
static constexpr uint8_t ccInvalidSessionId = 0x87;
static constexpr uint8_t ccInvalidSessionHandle = 0x88;
static constexpr uint8_t searchCurrentSession = 0;
static constexpr uint8_t searchSessionByHandle = 0xFE;
static constexpr uint8_t searchSessionById = 0xFF;
// MSB BIT 7 BIT 6 assigned for netipmid instance in session handle.
static constexpr uint8_t multiIntfaceSessionHandleMask = 0x3F;

// MSB BIT 31-BIT30 assigned for netipmid instance in session ID
static constexpr uint32_t multiIntfaceSessionIDMask = 0x3FFFFFFF;

enum class State : uint8_t
{
    inactive,           // Session is not in use
    setupInProgress,    // Session Setup Sequence is progressing
    active,             // Session is active
    tearDownInProgress, // When Closing Session
};

} // namespace session
