#include "example/example.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <string>
#include <vector>

namespace blobs
{

using namespace phosphor::logging;

constexpr char ExampleBlobHandler::supportedPath[];

ExampleBlob* ExampleBlobHandler::getSession(uint16_t id)
{
    auto search = sessions.find(id);
    if (search == sessions.end())
    {
        return nullptr;
    }
    /* Not thread-safe, however, the blob handler deliberately assumes serial
     * execution. */
    return &search->second;
}

bool ExampleBlobHandler::canHandleBlob(const std::string& path)
{
    return (path == supportedPath);
}

std::vector<std::string> ExampleBlobHandler::getBlobIds()
{
    return {supportedPath};
}

bool ExampleBlobHandler::deleteBlob(const std::string& path)
{
    return false;
}

bool ExampleBlobHandler::stat(const std::string& path, BlobMeta* meta)
{
    return false;
}

bool ExampleBlobHandler::open(uint16_t session, uint16_t flags,
                              const std::string& path)
{
    if (!canHandleBlob(path))
    {
        return false;
    }

    auto findSess = sessions.find(session);
    if (findSess != sessions.end())
    {
        /* This session is already active. */
        return false;
    }
    sessions[session] = ExampleBlob(session, flags);
    return true;
}

std::vector<uint8_t> ExampleBlobHandler::read(uint16_t session, uint32_t offset,
                                              uint32_t requestedSize)
{
    ExampleBlob* sess = getSession(session);
    if (!sess)
    {
        return std::vector<uint8_t>();
    }

    /* Is the offset beyond the array? */
    if (offset >= sizeof(sess->buffer))
    {
        return std::vector<uint8_t>();
    }

    /* Determine how many bytes we can read from the offset.
     * In this case, if they read beyond "size" we allow it.
     */
    uint32_t remain = sizeof(sess->buffer) - offset;
    uint32_t numBytes = std::min(remain, requestedSize);
    /* Copy the bytes! */
    std::vector<uint8_t> result(numBytes);
    std::memcpy(result.data(), &sess->buffer[offset], numBytes);
    return result;
}

bool ExampleBlobHandler::write(uint16_t session, uint32_t offset,
                               const std::vector<uint8_t>& data)
{
    ExampleBlob* sess = getSession(session);
    if (!sess)
    {
        return false;
    }
    /* Is the offset beyond the array? */
    if (offset >= sizeof(sess->buffer))
    {
        return false;
    }
    /* Determine whether all their bytes will fit. */
    uint32_t remain = sizeof(sess->buffer) - offset;
    if (data.size() > remain)
    {
        return false;
    }
    sess->length =
        std::max(offset + data.size(),
                 static_cast<std::vector<uint8_t>::size_type>(sess->length));
    std::memcpy(&sess->buffer[offset], data.data(), data.size());
    return true;
}

bool ExampleBlobHandler::writeMeta(uint16_t session, uint32_t offset,
                                   const std::vector<uint8_t>& data)
{
    /* Not supported. */
    return false;
}

bool ExampleBlobHandler::commit(uint16_t session,
                                const std::vector<uint8_t>& data)
{
    ExampleBlob* sess = getSession(session);
    if (!sess)
    {
        return false;
    }

    /* Do something with the staged data!. */

    return false;
}

bool ExampleBlobHandler::close(uint16_t session)
{
    ExampleBlob* sess = getSession(session);
    if (!sess)
    {
        return false;
    }

    sessions.erase(session);
    return true;
}

bool ExampleBlobHandler::stat(uint16_t session, BlobMeta* meta)
{
    ExampleBlob* sess = getSession(session);
    if (!sess)
    {
        return false;
    }
    if (!meta)
    {
        return false;
    }
    meta->size = sess->length;
    meta->blobState = sess->state;
    return true;
}

bool ExampleBlobHandler::expire(uint16_t session)
{
    ExampleBlob* sess = getSession(session);
    if (!sess)
    {
        return false;
    }
    /* TODO: implement session expiration behavior. */
    return false;
}

void setupExampleHandler() __attribute__((constructor));

void setupExampleHandler()
{
    // You don't need to do anything in the constructor.
}

} // namespace blobs

/**
 * This method is required by the blob manager.
 *
 * It is called to grab a handler for registering the blob handler instance.
 */
std::unique_ptr<blobs::GenericBlobInterface> createHandler()
{
    return std::make_unique<blobs::ExampleBlobHandler>();
}
