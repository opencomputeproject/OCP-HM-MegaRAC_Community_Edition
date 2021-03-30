#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace blobs
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

enum OpenFlags
{
    read = (1 << 0),
    write = (1 << 1),
    /* bits 3-7 reserved. */
    /* bits 8-15 given blob-specific definitions */
};

enum StateFlags
{
    open_read = (1 << 0),
    open_write = (1 << 1),
    committing = (1 << 2),
    committed = (1 << 3),
    commit_error = (1 << 4),
};

struct BlobMeta
{
    uint16_t blobState;
    uint32_t size;
    std::vector<uint8_t> metadata;

    bool operator==(const BlobMeta& rhs) const
    {
        return (this->blobState == rhs.blobState && this->size == rhs.size &&
                this->metadata == rhs.metadata);
    }
};

/*
 * All blob specific objects implement this interface.
 */
class GenericBlobInterface
{
  public:
    virtual ~GenericBlobInterface() = default;

    /**
     * Checks if the handler will manage this file path.
     *
     * @param[in] blobId.
     * @return bool whether it will manage the file path.
     */
    virtual bool canHandleBlob(const std::string& path) = 0;

    /**
     * Return the name(s) of the blob(s).  Used during GetCount.
     *
     * @return List of blobIds this handler manages.
     */
    virtual std::vector<std::string> getBlobIds() = 0;

    /**
     * Attempt to delete the blob specified by the path.
     *
     * @param[in] path - the blobId to try and delete.
     * @return bool - whether it was able to delete the blob.
     */
    virtual bool deleteBlob(const std::string& path) = 0;

    /**
     * Return metadata about the blob.
     *
     * @param[in] path - the blobId for metadata.
     * @param[in,out] meta - a pointer to a blobmeta.
     * @return bool - true if it was successful.
     */
    virtual bool stat(const std::string& path, BlobMeta* meta) = 0;

    /* The methods below are per session. */

    /**
     * Attempt to open a session from this path.
     *
     * @param[in] session - the session id.
     * @param[in] flags - the open flags.
     * @param[in] path - the blob path.
     * @return bool - was able to open the session.
     */
    virtual bool open(uint16_t session, uint16_t flags,
                      const std::string& path) = 0;

    /**
     * Attempt to read from a blob.
     *
     * @param[in] session - the session id.
     * @param[in] offset - offset into the blob.
     * @param[in] requestedSize - number of bytes to read.
     * @return Bytes read back (0 length on error).
     */
    virtual std::vector<uint8_t> read(uint16_t session, uint32_t offset,
                                      uint32_t requestedSize) = 0;

    /**
     * Attempt to write to a blob.
     *
     * @param[in] session - the session id.
     * @param[in] offset - offset into the blob.
     * @param[in] data - the data to write.
     * @return bool - was able to write.
     */
    virtual bool write(uint16_t session, uint32_t offset,
                       const std::vector<uint8_t>& data) = 0;

    /**
     * Attempt to write metadata to a blob.
     *
     * @param[in] session - the session id.
     * @param[in] offset - offset into the blob.
     * @param[in] data - the data to write.
     * @return bool - was able to write.
     */
    virtual bool writeMeta(uint16_t session, uint32_t offset,
                           const std::vector<uint8_t>& data) = 0;

    /**
     * Attempt to commit to a blob.
     *
     * @param[in] session - the session id.
     * @param[in] data - optional commit data.
     * @return bool - was able to start commit.
     */
    virtual bool commit(uint16_t session, const std::vector<uint8_t>& data) = 0;

    /**
     * Attempt to close your session.
     *
     * @param[in] session - the session id.
     * @return bool - was able to close session.
     */
    virtual bool close(uint16_t session) = 0;

    /**
     * Attempt to return metadata for the session's view of the blob.
     *
     * @param[in] session - the session id.
     * @param[in,out] meta - pointer to update with the BlobMeta.
     * @return bool - wether it was successful.
     */
    virtual bool stat(uint16_t session, BlobMeta* meta) = 0;

    /**
     * Attempt to expire a session.  This is called when a session has been
     * inactive for at least 10 minutes.
     *
     * @param[in] session - the session id.
     * @return bool - whether the session was able to be closed.
     */
    virtual bool expire(uint16_t session) = 0;
};
} // namespace blobs

#ifdef __cplusplus
extern "C" {
#endif

/**
 * All Blob handlers need to implement this method.  It is called after loading
 * the library to then get a handle to the blob handler.
 *
 * @return a unique pointer to your blob handler instance.
 */
std::unique_ptr<blobs::GenericBlobInterface> createHandler();

#ifdef __cplusplus
}
#endif
