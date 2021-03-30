#include "handler.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

namespace blobs
{

namespace internal
{

/**
 * @brief: Get baseId from a blob id string
 * @param blobId: Input blob id which is expected to only contain alphanumerical
 *                characters and '/'.
 * @returns: the baseId containing the blobId, stripping all contents from the
 *           last '/'. If no '/' is present, an empty string is returned.
 */
static std::string getBaseFromId(const std::string& blobId)
{
    return blobId.substr(0, blobId.find_last_of('/') + 1);
}

} // namespace internal

void BinaryStoreBlobHandler::addNewBinaryStore(
    std::unique_ptr<binstore::BinaryStoreInterface> store)
{
    // TODO: this is a very rough measure to test the mock interface for now.
    stores_[store->getBaseBlobId()] = std::move(store);
}

bool BinaryStoreBlobHandler::canHandleBlob(const std::string& path)
{
    auto base = internal::getBaseFromId(path);
    if (base.empty() || base == path)
    {
        /* Operations on baseId itself or an empty base is not allowed */
        return false;
    }

    return std::any_of(stores_.begin(), stores_.end(),
                       [&](const auto& baseStorePair) {
                           return base == baseStorePair.second->getBaseBlobId();
                       });
}

std::vector<std::string> BinaryStoreBlobHandler::getBlobIds()
{
    std::vector<std::string> result;

    for (const auto& baseStorePair : stores_)
    {
        const auto& ids = baseStorePair.second->getBlobIds();
        result.insert(result.end(), ids.begin(), ids.end());
    }

    return result;
}

bool BinaryStoreBlobHandler::deleteBlob(const std::string& path)
{
    auto it = stores_.find(internal::getBaseFromId(path));
    if (it == stores_.end())
    {
        return false;
    }

    return it->second->deleteBlob(path);
}

bool BinaryStoreBlobHandler::stat(const std::string& path,
                                  struct BlobMeta* meta)
{
    auto it = stores_.find(internal::getBaseFromId(path));
    if (it == stores_.end())
    {
        return false;
    }

    return it->second->stat(meta);
}

bool BinaryStoreBlobHandler::open(uint16_t session, uint16_t flags,
                                  const std::string& path)
{
    if (!canHandleBlob(path))
    {
        return false;
    }

    auto found = sessions_.find(session);
    if (found != sessions_.end())
    {
        /* This session is already active */
        return false;
    }

    const auto& base = internal::getBaseFromId(path);

    if (stores_.find(base) == stores_.end())
    {
        return false;
    }

    if (!stores_[base]->openOrCreateBlob(path, flags))
    {
        return false;
    }

    sessions_[session] = stores_[base].get();
    return true;
}

std::vector<uint8_t> BinaryStoreBlobHandler::read(uint16_t session,
                                                  uint32_t offset,
                                                  uint32_t requestedSize)
{
    auto it = sessions_.find(session);
    if (it == sessions_.end())
    {
        return std::vector<uint8_t>();
    }

    return it->second->read(offset, requestedSize);
}

bool BinaryStoreBlobHandler::write(uint16_t session, uint32_t offset,
                                   const std::vector<uint8_t>& data)
{
    auto it = sessions_.find(session);
    if (it == sessions_.end())
    {
        return false;
    }

    return it->second->write(offset, data);
}

bool BinaryStoreBlobHandler::writeMeta(uint16_t session, uint32_t offset,
                                       const std::vector<uint8_t>& data)
{
    /* Binary store handler doesn't support write meta */
    return false;
}

bool BinaryStoreBlobHandler::commit(uint16_t session,
                                    const std::vector<uint8_t>& data)
{
    auto it = sessions_.find(session);
    if (it == sessions_.end())
    {
        return false;
    }

    return it->second->commit();
}

bool BinaryStoreBlobHandler::close(uint16_t session)
{
    auto it = sessions_.find(session);
    if (it == sessions_.end())
    {
        return false;
    }

    if (!it->second->close())
    {
        return false;
    }

    sessions_.erase(session);
    return true;
}

bool BinaryStoreBlobHandler::stat(uint16_t session, struct BlobMeta* meta)
{
    auto it = sessions_.find(session);
    if (it == sessions_.end())
    {
        return false;
    }

    return it->second->stat(meta);
}

bool BinaryStoreBlobHandler::expire(uint16_t session)
{
    return close(session);
}

} // namespace blobs
