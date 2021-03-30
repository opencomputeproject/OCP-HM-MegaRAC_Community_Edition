/* Copyright 2018 Intel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef IPMBDEFINES_HPP
#define IPMBDEFINES_HPP

#include <inttypes.h>

#pragma pack(1)
typedef struct _IPMB_HEADER
{
    union
    {
        struct
        {
            /** @brief IPMB Connection Header Format */
            uint8_t address;
            uint8_t rsNetFnLUN; /// @brief responder's net function and logical
                                /// unit number
            uint8_t checksum1;  /// @brief checksum computed on first two bytes
                                /// of IPMB_HEADER
            /** @brief IPMB Header */
            uint8_t rqSA;     /// @brief requester's slave address, LS bit=0
            uint8_t rqSeqLUN; /// @brief requester's sequence number and logical
                              /// unit number
            uint8_t cmd; /// @brief command required by the network identify the
                         /// type of rqts
            uint8_t data[]; /// @brief payload
        } Req;              /// @brief IPMB request header
        struct
        {
            uint8_t address;
            /** @brief IPMB Connection Header Format */
            uint8_t rqNetFnLUN; /// @brief requester's net function and logical
                                /// unit number
            uint8_t checksum1;  /// @brief checksum computed on first two bytes
                                /// of IPMB_HEADER
            /** @brief IPMB Header */
            uint8_t rsSA;     /// @brief responder's slave address, LS bit=0
            uint8_t rsSeqLUN; /// @brief responder's sequence number and logical
                              /// unit number
            uint8_t cmd; /// @brief command required by the network identify the
                         /// type of rqts
            uint8_t completionCode; /// @brief IPMB nodes return a Completion
                                    /// Code in all response msgs
            uint8_t data[];         /// @brief payload
        } Resp;                     /// @brief IPMB response header
    } Header;                       /// @brief IPMB frame header
} IPMB_HEADER;
#pragma pack()

typedef struct _IPMB_DRV_HDR
{
    uint8_t len;
    IPMB_HEADER hdr;
} IPMB_PKT;

#endif
