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

#ifndef IPMBUTILS_HPP
#define IPMBUTILS_HPP

/**
 * @brief function performing IPMB header and data checksum validation
 */
bool isFrameValid(IPMB_HEADER *frame, uint8_t length);

/**
 * @brief function computing IPMB layer checksum
 */
uint8_t ipmbChecksumCompute(uint8_t *data, uint8_t length);

#endif
