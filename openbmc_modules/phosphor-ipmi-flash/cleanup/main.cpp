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

#include "config.h"

#include "cleanup.hpp"
#include "fs.hpp"
#include "util.hpp"

#include <blobs-ipmid/blobs.hpp>

#include <memory>
#include <string>
#include <vector>

namespace ipmi_flash
{
std::vector<std::string> files = {
    STATIC_HANDLER_STAGED_NAME, TARBALL_STAGED_NAME, HASH_FILENAME,
    VERIFY_STATUS_FILENAME, UPDATE_STATUS_FILENAME};
} // namespace ipmi_flash

extern "C" std::unique_ptr<blobs::GenericBlobInterface> createHandler()
{
    auto handler = ipmi_flash::FileCleanupHandler::CreateCleanupHandler(
        ipmi_flash::cleanupBlobId, ipmi_flash::files,
        std::make_unique<ipmi_flash::FileSystem>());

    if (!handler)
    {
        std::fprintf(stderr, "Unable to create FileCleanupHandle for Firmware");
        return nullptr;
    }

    return handler;
}
