#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ipmiblob
{

enum class BlobOEMCommands : std::uint8_t
{
    bmcBlobGetCount = 0,
    bmcBlobEnumerate = 1,
    bmcBlobOpen = 2,
    bmcBlobRead = 3,
    bmcBlobWrite = 4,
    bmcBlobCommit = 5,
    bmcBlobClose = 6,
    bmcBlobDelete = 7,
    bmcBlobStat = 8,
    bmcBlobSessionStat = 9,
    bmcBlobWriteMeta = 10,
};

struct StatResponse
{
    std::uint16_t blob_state;
    std::uint32_t size;
    std::vector<std::uint8_t> metadata;

    bool operator==(const StatResponse& rhs) const
    {
        return (this->blob_state == rhs.blob_state && this->size == rhs.size &&
                this->metadata == rhs.metadata);
    }
};

class BlobInterface
{
  public:
    virtual ~BlobInterface() = default;

    /**
     * Call commit on a blob.  The behavior here is up to the blob itself.
     *
     * @param[in] session - the session id.
     * @param[in] bytes - the bytes to send.
     * @throws BlobException on failure.
     */
    virtual void commit(std::uint16_t session,
                        const std::vector<std::uint8_t>& bytes) = 0;

    /**
     * Write metadata to a blob.
     *
     * @param[in] session - the session id.
     * @param[in] offset - the offset for the metadata to write.
     * @param[in] bytes - the bytes to send.
     * @throws BlobException on failure.
     */
    virtual void writeMeta(std::uint16_t session, std::uint32_t offset,
                           const std::vector<std::uint8_t>& bytes) = 0;

    /**
     * Write bytes to a blob.
     *
     * @param[in] session - the session id.
     * @param[in] offset - the offset to which to write the bytes.
     * @param[in] bytes - the bytes to send.
     * @throws BlobException on failure.
     */
    virtual void writeBytes(std::uint16_t session, std::uint32_t offset,
                            const std::vector<std::uint8_t>& bytes) = 0;

    /**
     * Get a list of the blob_ids provided by the BMC.
     *
     * @return list of strings, each representing a blob_id returned.
     */
    virtual std::vector<std::string> getBlobList() = 0;

    /**
     * Get the stat() on the blob_id.
     *
     * @param[in] id - the blob_id.
     * @return metadata structure.
     */
    virtual StatResponse getStat(const std::string& id) = 0;

    /**
     * Get the stat() on the blob session.
     *
     * @param[in] session - the blob session
     * @return metadata structure
     */
    virtual StatResponse getStat(std::uint16_t session) = 0;

    /**
     * Attempt to open the file using the specific data interface flag.
     *
     * @param[in] blob - the blob_id to open.
     * @param[in] handlerFlags - the data interface flag, if relevant.
     * @return the session id on success.
     * @throws BlobException on failure.
     */
    virtual std::uint16_t openBlob(const std::string& id,
                                   std::uint16_t handlerFlags) = 0;

    /**
     * Attempt to close the open session.
     *
     * @param[in] session - the session to close.
     */
    virtual void closeBlob(std::uint16_t session) = 0;

    /**
     * Attempt to delete a blobId.
     *
     * @param[in] path - the blobId path.
     */
    virtual void deleteBlob(const std::string& id) = 0;

    /**
     * Read bytes from a blob.
     *
     * @param[in] session - the session id.
     * @param[in] offset - the offset to which to write the bytes.
     * @param[in] length - the number of bytes to read.
     * @return the bytes read
     * @throws BlobException on failure.
     */
    virtual std::vector<std::uint8_t> readBytes(std::uint16_t session,
                                                std::uint32_t offset,
                                                std::uint32_t length) = 0;
};

} // namespace ipmiblob
