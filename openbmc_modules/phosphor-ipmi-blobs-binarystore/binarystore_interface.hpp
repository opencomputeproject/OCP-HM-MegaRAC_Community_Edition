#pragma once

#include <blobs-ipmid/blobs.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

namespace binstore
{

/**
 * @class BinaryStoreInterface is an abstraction for a storage location.
 *     Each instance would be uniquely identified by a baseId string.
 */
class BinaryStoreInterface
{
  public:
    virtual ~BinaryStoreInterface() = default;

    /**
     * @returns baseId string of the storage.
     */
    virtual std::string getBaseBlobId() const = 0;

    /**
     * @returns List of all open blob IDs, plus the base.
     */
    virtual std::vector<std::string> getBlobIds() const = 0;

    /**
     * Opens a blob given its name. If there is no one, create one.
     * @param blobId: The blob id to operate on.
     * @param flags: Either read flag or r/w flag has to be specified.
     * @returns True if open/create successfully.
     */
    virtual bool openOrCreateBlob(const std::string& blobId,
                                  uint16_t flags) = 0;

    /**
     * Deletes a blob given its name. If there is no one,
     * @param blobId: The blob id to operate on.
     * @returns True if deleted.
     */
    virtual bool deleteBlob(const std::string& blobId) = 0;

    /**
     * Reads data from the currently opened blob.
     * @param offset: offset into the blob to read
     * @param requestedSize: how many bytes to read
     * @returns Bytes able to read. Returns empty if nothing can be read or
     *          if there is no open blob.
     */
    virtual std::vector<uint8_t> read(uint32_t offset,
                                      uint32_t requestedSize) = 0;

    /**
     * Writes data to the currently openend blob.
     * @param offset: offset into the blob to write
     * @param data: bytes to write
     * @returns True if able to write the entire data successfully
     */
    virtual bool write(uint32_t offset, const std::vector<uint8_t>& data) = 0;

    /**
     * Commits data to the persistent storage specified during blob init.
     * @returns True if able to write data to sysfile successfully
     */
    virtual bool commit() = 0;

    /**
     * Closes blob, which prevents further modifications. Uncommitted data will
     * be lost.
     * @returns True if able to close the blob successfully
     */
    virtual bool close() = 0;

    /**
     * Returns blob stat flags.
     * @param meta: output stat flags.
     * @returns True if able to get the stat flags and write to *meta
     */
    virtual bool stat(blobs::BlobMeta* meta) = 0;
};

} // namespace binstore
