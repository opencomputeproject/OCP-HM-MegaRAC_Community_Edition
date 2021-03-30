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

#include "blob_handler.hpp"

#include "blob_errors.hpp"
#include "crc.hpp"
#include "ipmi_errors.hpp"
#include "ipmi_interface.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <memory>

namespace ipmiblob
{

namespace
{
const std::array<std::uint8_t, 3> ipmiPhosphorOen = {0xcf, 0xc2, 0x00};
}

std::unique_ptr<BlobInterface>
    BlobHandler::CreateBlobHandler(std::unique_ptr<IpmiInterface> ipmi)
{
    return std::make_unique<BlobHandler>(std::move(ipmi));
}

std::vector<std::uint8_t>
    BlobHandler::sendIpmiPayload(BlobOEMCommands command,
                                 const std::vector<std::uint8_t>& payload)
{
    std::vector<std::uint8_t> request, reply, bytes;

    std::copy(ipmiPhosphorOen.begin(), ipmiPhosphorOen.end(),
              std::back_inserter(request));
    request.push_back(static_cast<std::uint8_t>(command));

    if (payload.size() > 0)
    {
        /* Grow the vector to hold the bytes. */
        request.reserve(request.size() + sizeof(std::uint16_t));

        /* CRC required. */
        std::uint16_t crc = generateCrc(payload);
        auto src = reinterpret_cast<const std::uint8_t*>(&crc);

        std::copy(src, src + sizeof(crc), std::back_inserter(request));

        /* Copy the payload. */
        std::copy(payload.begin(), payload.end(), std::back_inserter(request));
    }

    try
    {
        reply = ipmi->sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, request);
    }
    catch (const IpmiException& e)
    {
        throw BlobException(e.what());
    }

    /* IPMI_CC was OK, and it returned no bytes, so let's be happy with that for
     * now.
     */
    if (reply.size() == 0)
    {
        return reply;
    }

    /* This cannot be a response because it's smaller than the smallest
     * response.
     */
    if (reply.size() < ipmiPhosphorOen.size())
    {
        throw BlobException("Invalid response length");
    }

    /* Verify the OEN. */
    if (std::memcmp(ipmiPhosphorOen.data(), reply.data(),
                    ipmiPhosphorOen.size()) != 0)
    {
        throw BlobException("Invalid OEN received");
    }

    /* In this case there was no data, as there was no CRC. */
    std::size_t headerSize = ipmiPhosphorOen.size() + sizeof(std::uint16_t);
    if (reply.size() < headerSize)
    {
        return {};
    }

    /* Validate CRC. */
    std::uint16_t crc;
    auto ptr = reinterpret_cast<std::uint8_t*>(&crc);
    std::memcpy(ptr, &reply[ipmiPhosphorOen.size()], sizeof(crc));

    bytes.insert(bytes.begin(), reply.begin() + headerSize, reply.end());

    auto computed = generateCrc(bytes);
    if (crc != computed)
    {
        std::fprintf(stderr, "Invalid CRC, received: 0x%x, computed: 0x%x\n",
                     crc, computed);
        throw BlobException("Invalid CRC on received data.");
    }

    return bytes;
}

int BlobHandler::getBlobCount()
{
    std::uint32_t count;
    try
    {
        auto resp = sendIpmiPayload(BlobOEMCommands::bmcBlobGetCount, {});
        if (resp.size() != sizeof(count))
        {
            return 0;
        }

        /* LE to LE (need to make this portable as some point. */
        std::memcpy(&count, resp.data(), sizeof(count));
    }
    catch (const BlobException& b)
    {
        return 0;
    }

    return count;
}

std::string BlobHandler::enumerateBlob(std::uint32_t index)
{
    std::vector<std::uint8_t> payload;
    auto data = reinterpret_cast<const std::uint8_t*>(&index);

    std::copy(data, data + sizeof(std::uint32_t), std::back_inserter(payload));

    try
    {
        auto resp = sendIpmiPayload(BlobOEMCommands::bmcBlobEnumerate, payload);
        return (resp.size() > 0) ? std::string(&resp[0], &resp[resp.size() - 1])
                                 : "";
    }
    catch (const BlobException& b)
    {
        return "";
    }
}

void BlobHandler::commit(std::uint16_t session,
                         const std::vector<std::uint8_t>& bytes)
{
    std::vector<std::uint8_t> request;
    auto addrSession = reinterpret_cast<const std::uint8_t*>(&session);
    std::copy(addrSession, addrSession + sizeof(session),
              std::back_inserter(request));

    /* You have one byte to describe the length. */
    if (bytes.size() > std::numeric_limits<std::uint8_t>::max())
    {
        throw BlobException("Commit data length greater than 8-bit limit\n");
    }

    std::uint8_t length = static_cast<std::uint8_t>(bytes.size());
    auto addrLength = reinterpret_cast<const std::uint8_t*>(&length);
    std::copy(addrLength, addrLength + sizeof(length),
              std::back_inserter(request));

    std::copy(bytes.begin(), bytes.end(), std::back_inserter(request));

    sendIpmiPayload(BlobOEMCommands::bmcBlobCommit, request);
}

void BlobHandler::writeGeneric(BlobOEMCommands command, std::uint16_t session,
                               std::uint32_t offset,
                               const std::vector<std::uint8_t>& bytes)
{
    std::vector<std::uint8_t> payload;

    payload.reserve(sizeof(std::uint16_t) + sizeof(std::uint32_t) +
                    bytes.size());

    auto data = reinterpret_cast<const std::uint8_t*>(&session);
    std::copy(data, data + sizeof(std::uint16_t), std::back_inserter(payload));

    data = reinterpret_cast<const std::uint8_t*>(&offset);
    std::copy(data, data + sizeof(std::uint32_t), std::back_inserter(payload));

    std::copy(bytes.begin(), bytes.end(), std::back_inserter(payload));

    sendIpmiPayload(command, payload);
}

void BlobHandler::writeMeta(std::uint16_t session, std::uint32_t offset,
                            const std::vector<std::uint8_t>& bytes)
{
    writeGeneric(BlobOEMCommands::bmcBlobWriteMeta, session, offset, bytes);
}

void BlobHandler::writeBytes(std::uint16_t session, std::uint32_t offset,
                             const std::vector<std::uint8_t>& bytes)
{
    writeGeneric(BlobOEMCommands::bmcBlobWrite, session, offset, bytes);
}

std::vector<std::string> BlobHandler::getBlobList()
{
    std::vector<std::string> list;
    int blobCount = getBlobCount();

    for (int i = 0; i < blobCount; i++)
    {
        auto name = enumerateBlob(i);
        /* Currently ignore failures. */
        if (!name.empty())
        {
            list.push_back(name);
        }
    }

    return list;
}

StatResponse BlobHandler::statGeneric(BlobOEMCommands command,
                                      const std::vector<std::uint8_t>& request)
{
    StatResponse meta;
    static constexpr std::size_t blobStateSize = sizeof(meta.blob_state);
    static constexpr std::size_t metaSize = sizeof(meta.size);
    static constexpr std::size_t metaOffset = blobStateSize + metaSize;
    static constexpr std::size_t minRespSize =
        metaOffset + sizeof(std::uint8_t);
    std::vector<std::uint8_t> resp;

    try
    {
        resp = sendIpmiPayload(command, request);
    }
    catch (const BlobException& b)
    {
        throw;
    }

    // Avoid out of bounds memcpy below
    if (resp.size() < minRespSize)
    {
        std::fprintf(stderr,
                     "Invalid response length, Got %li which is less than "
                     "minRespSize %li\n",
                     resp.size(), minRespSize);
        throw BlobException("Invalid response length");
    }

    std::memcpy(&meta.blob_state, &resp[0], blobStateSize);
    std::memcpy(&meta.size, &resp[blobStateSize], metaSize);
    std::uint8_t len = resp[metaOffset];

    auto metaDataLength = resp.size() - minRespSize;
    if (metaDataLength != len)
    {
        std::fprintf(stderr,
                     "Metadata length did not match actual length, Got %li "
                     "which does not equal expected length %i\n",
                     metaDataLength, len);
        throw BlobException("Metadata length did not match actual length");
    }

    if (len > 0)
    {
        meta.metadata.resize(len);
        std::copy(resp.begin() + minRespSize, resp.end(),
                  meta.metadata.begin());
    }

    return meta;
}

StatResponse BlobHandler::getStat(const std::string& id)
{
    std::vector<std::uint8_t> name;
    std::copy(id.begin(), id.end(), std::back_inserter(name));
    name.push_back(0x00); /* need to add nul-terminator. */

    return statGeneric(BlobOEMCommands::bmcBlobStat, name);
}

StatResponse BlobHandler::getStat(std::uint16_t session)
{
    std::vector<std::uint8_t> request;
    auto addrSession = reinterpret_cast<const std::uint8_t*>(&session);
    std::copy(addrSession, addrSession + sizeof(session),
              std::back_inserter(request));

    return statGeneric(BlobOEMCommands::bmcBlobSessionStat, request);
}

std::uint16_t BlobHandler::openBlob(const std::string& id,
                                    std::uint16_t handlerFlags)
{
    std::uint16_t session;
    std::vector<std::uint8_t> request, resp;
    auto addrFlags = reinterpret_cast<const std::uint8_t*>(&handlerFlags);

    std::copy(addrFlags, addrFlags + sizeof(handlerFlags),
              std::back_inserter(request));
    std::copy(id.begin(), id.end(), std::back_inserter(request));
    request.push_back(0x00); /* need to add nul-terminator. */

    try
    {
        resp = sendIpmiPayload(BlobOEMCommands::bmcBlobOpen, request);
    }
    catch (const BlobException& b)
    {
        throw;
    }

    if (resp.size() != sizeof(session))
    {
        throw BlobException("Did not receive session.");
    }

    std::memcpy(&session, resp.data(), sizeof(session));
    return session;
}

void BlobHandler::closeBlob(std::uint16_t session)
{
    std::vector<std::uint8_t> request;
    auto addrSession = reinterpret_cast<const std::uint8_t*>(&session);
    std::copy(addrSession, addrSession + sizeof(session),
              std::back_inserter(request));

    try
    {
        sendIpmiPayload(BlobOEMCommands::bmcBlobClose, request);
    }
    catch (const BlobException& b)
    {
        std::fprintf(stderr, "Received failure on close: %s\n", b.what());
    }
}

void BlobHandler::deleteBlob(const std::string& id)
{
    std::vector<std::uint8_t> name;
    std::copy(id.begin(), id.end(), std::back_inserter(name));
    name.push_back(0x00); /* need to add nul-terminator. */

    try
    {
        sendIpmiPayload(BlobOEMCommands::bmcBlobDelete, name);
    }
    catch (const BlobException& b)
    {
        std::fprintf(stderr, "Received failure on delete: %s\n", b.what());
    }
}

std::vector<std::uint8_t> BlobHandler::readBytes(std::uint16_t session,
                                                 std::uint32_t offset,
                                                 std::uint32_t length)
{
    std::vector<std::uint8_t> payload;

    payload.reserve(sizeof(std::uint16_t) + sizeof(std::uint32_t) +
                    sizeof(std::uint32_t));

    auto data = reinterpret_cast<const std::uint8_t*>(&session);
    std::copy(data, data + sizeof(std::uint16_t), std::back_inserter(payload));

    data = reinterpret_cast<const std::uint8_t*>(&offset);
    std::copy(data, data + sizeof(std::uint32_t), std::back_inserter(payload));

    data = reinterpret_cast<const std::uint8_t*>(&length);
    std::copy(data, data + sizeof(std::uint32_t), std::back_inserter(payload));

    return sendIpmiPayload(BlobOEMCommands::bmcBlobRead, payload);
}

} // namespace ipmiblob
