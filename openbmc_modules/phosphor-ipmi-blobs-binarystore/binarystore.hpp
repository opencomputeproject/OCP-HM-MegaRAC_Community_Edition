#pragma once

#include "binarystore_interface.hpp"
#include "sys_file.hpp"

#include <unistd.h>

#include <blobs-ipmid/blobs.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "binaryblob.pb.h"

using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

namespace binstore
{

/**
 * @class BinaryStore instantiates a concrete implementation of
 *     BinaryStoreInterface. The dependency on file is injected through its
 *     constructor.
 */
class BinaryStore : public BinaryStoreInterface
{
  public:
    /* |CommitState| differs slightly with |StateFlags| in blob.hpp,
     * and thus is defined in the OEM space (bit 8 - 15). User can call stat()
     * to query the |CommitState| of the blob path. */
    enum CommitState
    {
        Dirty = (1 << 8), // In-memory data might not match persisted data
        Clean = (1 << 9), // In-memory data matches persisted data
        Uninitialized = (1 << 10), // Cannot find persisted data
        CommitError = (1 << 11)    // Error happened during committing
    };

    BinaryStore() = delete;
    BinaryStore(const std::string& baseBlobId, std::unique_ptr<SysFile> file) :
        baseBlobId_(baseBlobId), file_(std::move(file))
    {
        blob_.set_blob_base_id(baseBlobId_);
    }

    ~BinaryStore() = default;

    BinaryStore(const BinaryStore&) = delete;
    BinaryStore& operator=(const BinaryStore&) = delete;
    BinaryStore(BinaryStore&&) = default;
    BinaryStore& operator=(BinaryStore&&) = default;

    std::string getBaseBlobId() const override;
    std::vector<std::string> getBlobIds() const override;
    bool openOrCreateBlob(const std::string& blobId, uint16_t flags) override;
    bool deleteBlob(const std::string& blobId) override;
    std::vector<uint8_t> read(uint32_t offset, uint32_t requestedSize) override;
    bool write(uint32_t offset, const std::vector<uint8_t>& data) override;
    bool commit() override;
    bool close() override;
    bool stat(blobs::BlobMeta* meta) override;

    /**
     * Helper factory method to create a BinaryStore instance
     * @param baseBlobId: base id for the created instance
     * @param sysFile: system file object for storing binary
     * @returns unique_ptr to constructed BinaryStore. Caller should take
     *     ownership of the instance.
     */
    static std::unique_ptr<BinaryStoreInterface>
        createFromConfig(const std::string& baseBlobId,
                         std::unique_ptr<SysFile> file);

  private:
    /* Load the serialized data from sysfile if commit state is dirty.
     * Returns False if encountered error when loading */
    bool loadSerializedData();

    std::string baseBlobId_;
    binaryblobproto::BinaryBlobBase blob_;
    binaryblobproto::BinaryBlob* currentBlob_ = nullptr;
    bool writable_ = false;
    std::unique_ptr<SysFile> file_ = nullptr;
    CommitState commitState_ = CommitState::Dirty;
};

} // namespace binstore
