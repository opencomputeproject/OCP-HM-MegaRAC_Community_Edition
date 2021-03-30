#pragma once

#include "binarystore.hpp"

#include <blobs-ipmid/blobs.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

namespace blobs
{

class BinaryStoreBlobHandler : public GenericBlobInterface
{
  public:
    BinaryStoreBlobHandler() = default;
    ~BinaryStoreBlobHandler() = default;
    BinaryStoreBlobHandler(const BinaryStoreBlobHandler&) = delete;
    BinaryStoreBlobHandler& operator=(const BinaryStoreBlobHandler&) = delete;
    BinaryStoreBlobHandler(BinaryStoreBlobHandler&&) = default;
    BinaryStoreBlobHandler& operator=(BinaryStoreBlobHandler&&) = default;

    bool canHandleBlob(const std::string& path) override;
    std::vector<std::string> getBlobIds() override;
    bool deleteBlob(const std::string& path) override;
    bool stat(const std::string& path, struct BlobMeta* meta) override;
    bool open(uint16_t session, uint16_t flags,
              const std::string& path) override;
    std::vector<uint8_t> read(uint16_t session, uint32_t offset,
                              uint32_t requestedSize) override;
    bool write(uint16_t session, uint32_t offset,
               const std::vector<uint8_t>& data) override;
    bool writeMeta(uint16_t session, uint32_t offset,
                   const std::vector<uint8_t>& data) override;
    bool commit(uint16_t session, const std::vector<uint8_t>& data) override;
    bool close(uint16_t session) override;
    bool stat(uint16_t session, struct BlobMeta* meta) override;
    bool expire(uint16_t session) override;

    /**
     * Registers a binarystore in the main handler. Once called, handler will
     * take over the ownership of of enclosed binary store.
     *
     * @param store: pointer to a valid BinaryStore.
     * TODO: a minimal amount of error checking would be better
     */
    void addNewBinaryStore(
        std::unique_ptr<binstore::BinaryStoreInterface> store);

  private:
    /* map of baseId: binaryStore, which has a 1:1 relationship. */
    std::map<std::string, std::unique_ptr<binstore::BinaryStoreInterface>>
        stores_;

    /* map of sessionId: open binaryStore pointer. */
    std::unordered_map<uint16_t, binstore::BinaryStoreInterface*> sessions_;
};

} // namespace blobs
