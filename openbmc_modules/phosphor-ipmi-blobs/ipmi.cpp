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

#include "ipmi.hpp"

#include <cstring>
#include <string>
#include <unordered_map>

namespace blobs
{

bool validateRequestLength(BlobOEMCommands command, size_t requestLen)
{
    /* The smallest string is one letter and the nul-terminator. */
    static const int kMinStrLen = 2;

    static const std::unordered_map<BlobOEMCommands, size_t> minimumLengths = {
        {BlobOEMCommands::bmcBlobEnumerate, sizeof(struct BmcBlobEnumerateTx)},
        {BlobOEMCommands::bmcBlobOpen,
         sizeof(struct BmcBlobOpenTx) + kMinStrLen},
        {BlobOEMCommands::bmcBlobClose, sizeof(struct BmcBlobCloseTx)},
        {BlobOEMCommands::bmcBlobDelete,
         sizeof(struct BmcBlobDeleteTx) + kMinStrLen},
        {BlobOEMCommands::bmcBlobStat,
         sizeof(struct BmcBlobStatTx) + kMinStrLen},
        {BlobOEMCommands::bmcBlobSessionStat,
         sizeof(struct BmcBlobSessionStatTx)},
        {BlobOEMCommands::bmcBlobCommit, sizeof(struct BmcBlobCommitTx)},
        {BlobOEMCommands::bmcBlobRead, sizeof(struct BmcBlobReadTx)},
        {BlobOEMCommands::bmcBlobWrite,
         sizeof(struct BmcBlobWriteTx) + sizeof(uint8_t)},
        {BlobOEMCommands::bmcBlobWriteMeta,
         sizeof(struct BmcBlobWriteMetaTx) + sizeof(uint8_t)},
    };

    auto results = minimumLengths.find(command);
    if (results == minimumLengths.end())
    {
        /* Valid length by default if we don't care. */
        return true;
    }

    /* If the request is shorter than the minimum, it's invalid. */
    if (requestLen < results->second)
    {
        return false;
    }

    return true;
}

std::string stringFromBuffer(const char* start, size_t length)
{
    if (!start)
    {
        return "";
    }

    auto end = static_cast<const char*>(std::memchr(start, '\0', length));
    if (end != &start[length - 1])
    {
        return "";
    }

    return (end == nullptr) ? std::string() : std::string(start, end);
}

ipmi_ret_t getBlobCount(ManagerInterface* mgr, const uint8_t* reqBuf,
                        uint8_t* replyCmdBuf, size_t* dataLen)
{
    struct BmcBlobCountRx resp;
    resp.crc = 0;
    resp.blobCount = mgr->buildBlobList();

    /* Copy the response into the reply buffer */
    std::memcpy(replyCmdBuf, &resp, sizeof(resp));
    (*dataLen) = sizeof(resp);

    return IPMI_CC_OK;
}

ipmi_ret_t enumerateBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                         uint8_t* replyCmdBuf, size_t* dataLen)
{
    /* Verify datalen is >= sizeof(request) */
    struct BmcBlobEnumerateTx request;
    auto reply = reinterpret_cast<struct BmcBlobEnumerateRx*>(replyCmdBuf);
    (*dataLen) = 0;

    std::memcpy(&request, reqBuf, sizeof(request));

    std::string blobId = mgr->getBlobId(request.blobIdx);
    if (blobId == "")
    {
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    /* TODO(venture): Need to do a hard-code check against the maximum
     * reply buffer size. */
    reply->crc = 0;
    /* Explicilty copies the NUL-terminator. */
    std::memcpy(&reply->blobId, blobId.c_str(), blobId.length() + 1);

    (*dataLen) = sizeof(reply->crc) + blobId.length() + 1;

    return IPMI_CC_OK;
}

ipmi_ret_t openBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                    uint8_t* replyCmdBuf, size_t* dataLen)
{
    size_t requestLen = (*dataLen);
    auto request = reinterpret_cast<const struct BmcBlobOpenTx*>(reqBuf);
    uint16_t session;
    (*dataLen) = 0;

    std::string path = stringFromBuffer(
        request->blobId, (requestLen - sizeof(struct BmcBlobOpenTx)));
    if (path.empty())
    {
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    /* Attempt to open. */
    if (!mgr->open(request->flags, path, &session))
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    struct BmcBlobOpenRx reply;
    reply.crc = 0;
    reply.sessionId = session;

    std::memcpy(replyCmdBuf, &reply, sizeof(reply));
    (*dataLen) = sizeof(reply);

    return IPMI_CC_OK;
}

ipmi_ret_t closeBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                     uint8_t* replyCmdBuf, size_t* dataLen)
{
    struct BmcBlobCloseTx request;
    std::memcpy(&request, reqBuf, sizeof(request));
    (*dataLen) = 0;

    /* Attempt to close. */
    if (!mgr->close(request.sessionId))
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

ipmi_ret_t deleteBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                      uint8_t* replyCmdBuf, size_t* dataLen)
{
    size_t requestLen = (*dataLen);
    auto request = reinterpret_cast<const struct BmcBlobDeleteTx*>(reqBuf);
    (*dataLen) = 0;

    std::string path = stringFromBuffer(
        request->blobId, (requestLen - sizeof(struct BmcBlobDeleteTx)));
    if (path.empty())
    {
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    /* Attempt to delete. */
    if (!mgr->deleteBlob(path))
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

static ipmi_ret_t returnStatBlob(BlobMeta* meta, uint8_t* replyCmdBuf,
                                 size_t* dataLen)
{
    struct BmcBlobStatRx reply;
    reply.crc = 0;
    reply.blobState = meta->blobState;
    reply.size = meta->size;
    reply.metadataLen = meta->metadata.size();

    std::memcpy(replyCmdBuf, &reply, sizeof(reply));

    /* If there is metadata, copy it over. */
    if (meta->metadata.size())
    {
        uint8_t* metadata = &replyCmdBuf[sizeof(reply)];
        std::memcpy(metadata, meta->metadata.data(), reply.metadataLen);
    }

    (*dataLen) = sizeof(reply) + reply.metadataLen;
    return IPMI_CC_OK;
}

ipmi_ret_t statBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                    uint8_t* replyCmdBuf, size_t* dataLen)
{
    size_t requestLen = (*dataLen);
    auto request = reinterpret_cast<const struct BmcBlobStatTx*>(reqBuf);
    (*dataLen) = 0;

    std::string path = stringFromBuffer(
        request->blobId, (requestLen - sizeof(struct BmcBlobStatTx)));
    if (path.empty())
    {
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    /* Attempt to stat. */
    BlobMeta meta;
    if (!mgr->stat(path, &meta))
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return returnStatBlob(&meta, replyCmdBuf, dataLen);
}

ipmi_ret_t sessionStatBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                           uint8_t* replyCmdBuf, size_t* dataLen)
{
    struct BmcBlobSessionStatTx request;
    std::memcpy(&request, reqBuf, sizeof(request));
    (*dataLen) = 0;

    /* Attempt to stat. */
    BlobMeta meta;

    if (!mgr->stat(request.sessionId, &meta))
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return returnStatBlob(&meta, replyCmdBuf, dataLen);
}

ipmi_ret_t commitBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                      uint8_t* replyCmdBuf, size_t* dataLen)
{
    size_t requestLen = (*dataLen);
    auto request = reinterpret_cast<const struct BmcBlobCommitTx*>(reqBuf);
    (*dataLen) = 0;

    /* Sanity check the commitDataLen */
    if (request->commitDataLen > (requestLen - sizeof(struct BmcBlobCommitTx)))
    {
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::vector<uint8_t> data(request->commitDataLen);
    std::memcpy(data.data(), request->commitData, request->commitDataLen);

    if (!mgr->commit(request->sessionId, data))
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

ipmi_ret_t readBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                    uint8_t* replyCmdBuf, size_t* dataLen)
{
    struct BmcBlobReadTx request;
    std::memcpy(&request, reqBuf, sizeof(request));

    /* TODO(venture): Verify requestedSize can fit in a returned IPMI packet.
     */

    std::vector<uint8_t> result =
        mgr->read(request.sessionId, request.offset, request.requestedSize);

    /* If the Read fails, it returns success but with only the crc and 0 bytes
     * of data.
     * If there was data returned, copy into the reply buffer.
     */
    (*dataLen) = sizeof(struct BmcBlobReadRx);

    if (result.size())
    {
        uint8_t* output = &replyCmdBuf[sizeof(struct BmcBlobReadRx)];
        std::memcpy(output, result.data(), result.size());

        (*dataLen) = sizeof(struct BmcBlobReadRx) + result.size();
    }

    return IPMI_CC_OK;
}

ipmi_ret_t writeBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                     uint8_t* replyCmdBuf, size_t* dataLen)
{
    size_t requestLen = (*dataLen);
    auto request = reinterpret_cast<const struct BmcBlobWriteTx*>(reqBuf);
    (*dataLen) = 0;

    uint32_t size = requestLen - sizeof(struct BmcBlobWriteTx);
    std::vector<uint8_t> data(size);

    std::memcpy(data.data(), request->data, size);

    /* Attempt to write the bytes. */
    if (!mgr->write(request->sessionId, request->offset, data))
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

ipmi_ret_t writeMeta(ManagerInterface* mgr, const uint8_t* reqBuf,
                     uint8_t* replyCmdBuf, size_t* dataLen)
{
    size_t requestLen = (*dataLen);
    struct BmcBlobWriteMetaTx request;
    (*dataLen) = 0;

    /* Copy over the request. */
    std::memcpy(&request, reqBuf, sizeof(request));

    /* Determine number of bytes of metadata to write. */
    uint32_t size = requestLen - sizeof(request);

    /* Nothing really else to validate, we just copy those bytes. */
    std::vector<uint8_t> data(size);
    std::memcpy(data.data(), &reqBuf[sizeof(request)], size);

    /* Attempt to write the bytes. */
    if (!mgr->writeMeta(request.sessionId, request.offset, data))
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

} // namespace blobs
