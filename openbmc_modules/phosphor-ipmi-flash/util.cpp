/*
 * Copyright 2019 Google Inc.
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
 */

#include "util.hpp"

namespace ipmi_flash
{

const std::string biosBlobId = "/flash/bios";
const std::string updateBlobId = "/flash/update";
const std::string verifyBlobId = "/flash/verify";
const std::string hashBlobId = "/flash/hash";
const std::string activeImageBlobId = "/flash/active/image";
const std::string activeHashBlobId = "/flash/active/hash";
const std::string staticLayoutBlobId = "/flash/image";
const std::string ubiTarballBlobId = "/flash/tarball";
const std::string cleanupBlobId = "/flash/cleanup";

} // namespace ipmi_flash
