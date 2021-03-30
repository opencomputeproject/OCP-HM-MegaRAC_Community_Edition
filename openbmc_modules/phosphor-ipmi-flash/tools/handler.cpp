/*
 * Copyright 2018 Google Inc.
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

#include "handler.hpp"

#include "flags.hpp"
#include "helper.hpp"
#include "status.hpp"
#include "tool_errors.hpp"
#include "util.hpp"

#include <ipmiblob/blob_errors.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace host_tool
{

bool UpdateHandler::checkAvailable(const std::string& goalFirmware)
{
    std::vector<std::string> blobs = blob->getBlobList();

    auto blobInst = std::find_if(
        blobs.begin(), blobs.end(), [&goalFirmware](const std::string& iter) {
            /* Running into weird scenarios where the string comparison doesn't
             * work.  TODO: revisit.
             */
            return (0 == std::memcmp(goalFirmware.c_str(), iter.c_str(),
                                     goalFirmware.length()));
            // return (goalFirmware.compare(iter));
        });
    if (blobInst == blobs.end())
    {
        std::fprintf(stderr, "%s not found\n", goalFirmware.c_str());
        return false;
    }

    return true;
}

void UpdateHandler::sendFile(const std::string& target, const std::string& path)
{
    std::uint16_t session;
    auto supported = handler->supportedType();

    try
    {
        session = blob->openBlob(
            target, static_cast<std::uint16_t>(supported) |
                        static_cast<std::uint16_t>(
                            ipmi_flash::FirmwareFlags::UpdateFlags::openWrite));
    }
    catch (const ipmiblob::BlobException& b)
    {
        throw ToolException("blob exception received: " +
                            std::string(b.what()));
    }

    if (!handler->sendContents(path, session))
    {
        /* Need to close the session on failure, or it's stuck open (until the
         * blob handler timeout is implemented, and even then, why make it wait.
         */
        blob->closeBlob(session);
        throw ToolException("Failed to send contents of " + path);
    }

    blob->closeBlob(session);
}

bool UpdateHandler::verifyFile(const std::string& target, bool ignoreStatus)
{
    std::uint16_t session;
    bool success = false;

    try
    {
        session = blob->openBlob(
            target, static_cast<std::uint16_t>(
                        ipmi_flash::FirmwareFlags::UpdateFlags::openWrite));
    }
    catch (const ipmiblob::BlobException& b)
    {
        throw ToolException("blob exception received: " +
                            std::string(b.what()));
    }

    std::fprintf(stderr, "Committing to %s to trigger service\n",
                 target.c_str());

    try
    {
        blob->commit(session, {});
    }
    catch (const ipmiblob::BlobException& b)
    {
        blob->closeBlob(session);
        throw ToolException("blob exception received: " +
                            std::string(b.what()));
    }

    if (ignoreStatus)
    {
        // Skip checking the blob for status if ignoreStatus is enabled
        blob->closeBlob(session);
        return true;
    }

    std::fprintf(stderr, "Calling stat on %s session to check status\n",
                 target.c_str());

    if (pollStatus(session, blob))
    {
        std::fprintf(stderr, "Returned success\n");
        success = true;
    }
    else
    {
        std::fprintf(stderr, "Returned non-success (could still "
                             "be running (unlikely))\n");
    }

    blob->closeBlob(session);
    return (success == true);
}

void UpdateHandler::cleanArtifacts()
{
    /* open(), commit(), close() */
    std::uint16_t session;

    /* Errors aren't important for this call. */
    try
    {
        std::fprintf(stderr, "Opening the cleanup blob\n");
        session = blob->openBlob(
            ipmi_flash::cleanupBlobId,
            static_cast<std::uint16_t>(
                ipmi_flash::FirmwareFlags::UpdateFlags::openWrite));
    }
    catch (...)
    {
        return;
    }

    try
    {
        std::fprintf(stderr, "Committing to the cleanup blob\n");
        blob->commit(session, {});
        std::fprintf(stderr, "Closing cleanup blob\n");
    }
    catch (...)
    {}

    blob->closeBlob(session);
}

} // namespace host_tool
