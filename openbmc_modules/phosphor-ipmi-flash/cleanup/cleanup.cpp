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

#include "cleanup.hpp"

#include <blobs-ipmid/blobs.hpp>

#include <memory>
#include <string>
#include <vector>

namespace ipmi_flash
{

std::unique_ptr<blobs::GenericBlobInterface>
    FileCleanupHandler::CreateCleanupHandler(
        const std::string& blobId, const std::vector<std::string>& files,
        std::unique_ptr<FileSystemInterface> helper)
{
    return std::make_unique<FileCleanupHandler>(blobId, files,
                                                std::move(helper));
}

bool FileCleanupHandler::canHandleBlob(const std::string& path)
{
    return (path == supported);
}

std::vector<std::string> FileCleanupHandler::getBlobIds()
{
    return {supported};
}

bool FileCleanupHandler::commit(uint16_t session,
                                const std::vector<uint8_t>& data)
{
    for (const auto& file : files)
    {
        helper->remove(file);
    }

    return true;
}

} // namespace ipmi_flash
