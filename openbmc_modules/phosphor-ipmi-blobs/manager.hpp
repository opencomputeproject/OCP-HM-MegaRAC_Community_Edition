#pragma once

#include <blobs-ipmid/blobs.hpp>
#include <chrono>
#include <ctime>
#include <ipmid/oemrouter.hpp>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace blobs
{

using namespace std::chrono_literals;

/* The maximum read size.
 * NOTE: Once this can be dynamically determined, we'll switch to that method.
 * Having this in a header allows it to used cleanly for now.
 */
const int crcSize = sizeof(uint16_t);
const int btReplyHdrLen = 5;
const int btTransportLength = 64;
const uint32_t maximumReadSize =
    btTransportLength - (btReplyHdrLen + oem::groupMagicSize + crcSize);
constexpr auto defaultSessionTimeout = 10min;

struct SessionInfo
{
    SessionInfo() = default;
    SessionInfo(const std::string& path, GenericBlobInterface* handler,
                uint16_t flags) :
        blobId(path),
        handler(handler), flags(flags)
    {
    }
    ~SessionInfo() = default;

    std::string blobId;
    GenericBlobInterface* handler;
    uint16_t flags;

    /* Initially set during open(). read/write/writeMeta/commit/stat operations
     * would update it.
     */
    std::chrono::time_point<std::chrono::steady_clock> lastActionTime =
        std::chrono::steady_clock::now();
};

class ManagerInterface
{
  public:
    virtual ~ManagerInterface() = default;

    virtual bool
        registerHandler(std::unique_ptr<GenericBlobInterface> handler) = 0;

    virtual uint32_t buildBlobList() = 0;

    virtual std::string getBlobId(uint32_t index) = 0;

    virtual bool open(uint16_t flags, const std::string& path,
                      uint16_t* session) = 0;

    virtual bool stat(const std::string& path, BlobMeta* meta) = 0;

    virtual bool stat(uint16_t session, BlobMeta* meta) = 0;

    virtual bool commit(uint16_t session, const std::vector<uint8_t>& data) = 0;

    virtual bool close(uint16_t session) = 0;

    virtual std::vector<uint8_t> read(uint16_t session, uint32_t offset,
                                      uint32_t requestedSize) = 0;

    virtual bool write(uint16_t session, uint32_t offset,
                       const std::vector<uint8_t>& data) = 0;

    virtual bool deleteBlob(const std::string& path) = 0;

    virtual bool writeMeta(uint16_t session, uint32_t offset,
                           const std::vector<uint8_t>& data) = 0;
};

/**
 * Blob Manager used to store handlers and sessions.
 */
class BlobManager : public ManagerInterface
{
  public:
    BlobManager(std::chrono::seconds sessionTimeout = defaultSessionTimeout) :
        sessionTimeout(sessionTimeout)
    {
        next = static_cast<uint16_t>(std::time(nullptr));
    };

    ~BlobManager() = default;
    /* delete copy constructor & assignment operator, only support move
     * operations.
     */
    BlobManager(const BlobManager&) = delete;
    BlobManager& operator=(const BlobManager&) = delete;
    BlobManager(BlobManager&&) = default;
    BlobManager& operator=(BlobManager&&) = default;

    /**
     * Register a handler.  We own the pointer.
     *
     * @param[in] handler - a pointer to a blob handler.
     * @return bool - true if registered.
     */
    bool
        registerHandler(std::unique_ptr<GenericBlobInterface> handler) override;

    /**
     * Builds the blobId list for enumeration.
     *
     * @return lowest value returned is 0, otherwise the number of
     * blobIds.
     */
    uint32_t buildBlobList() override;

    /**
     * Grabs the blobId for the indexed blobId.
     *
     * @param[in] index - the index into the blobId cache.
     * @return string - the blobId or empty string on failure.
     */
    std::string getBlobId(uint32_t index) override;

    /**
     * Attempts to open the file specified and associates with a session.
     *
     * @param[in] flags - the flags to pass to open.
     * @param[in] path - the file path to open.
     * @param[in,out] session - pointer to store the session on success.
     * @return bool - true if able to open.
     */
    bool open(uint16_t flags, const std::string& path,
              uint16_t* session) override;

    /**
     * Attempts to retrieve a BlobMeta for the specified path.
     *
     * @param[in] path - the file path for stat().
     * @param[in,out] meta - a pointer to store the metadata.
     * @return bool - true if able to retrieve the information.
     */
    bool stat(const std::string& path, BlobMeta* meta) override;

    /**
     * Attempts to retrieve a BlobMeta for a given session.
     *
     * @param[in] session - the session for this command.
     * @param[in,out] meta - a pointer to store the metadata.
     * @return bool - true if able to retrieve the information.
     */
    bool stat(uint16_t session, BlobMeta* meta) override;

    /**
     * Attempt to commit a blob for a given session.
     *
     * @param[in] session - the session for this command.
     * @param[in] data - an optional commit blob.
     * @return bool - true if the commit succeeds.
     */
    bool commit(uint16_t session, const std::vector<uint8_t>& data) override;

    /**
     * Attempt to close a session.  If the handler returns a failure
     * in closing, the session is kept open.
     *
     * @param[in] session - the session for this command.
     * @return bool - true if the session was closed.
     */
    bool close(uint16_t session) override;

    /**
     * Attempt to read bytes from the blob.  If there's a failure, such as
     * an invalid offset it'll just return 0 bytes.
     *
     * @param[in] session - the session for this command.
     * @param[in] offset - the offset from which to read.
     * @param[in] requestedSize - the number of bytes to try and read.
     * @return the bytes read.
     */
    std::vector<uint8_t> read(uint16_t session, uint32_t offset,
                              uint32_t requestedSize) override;

    /**
     * Attempt to write to a blob.  The manager does not track whether
     * the session opened the file for writing.
     *
     * @param[in] session - the session for this command.
     * @param[in] offset - the offset into the blob to write.
     * @param[in] data - the bytes to write to the blob.
     * @return bool - true if the write succeeded.
     */
    bool write(uint16_t session, uint32_t offset,
               const std::vector<uint8_t>& data) override;

    /**
     * Attempt to delete a blobId.  This method will just call the
     * handler, which will return failure if the blob doesn't support
     * deletion.  This command will also fail if there are any open
     * sessions against the specific blob.
     *
     * In the case where they specify a folder, such as /blob/skm where
     * the "real" blobIds are /blob/skm/1, or /blob/skm/2, the manager
     * may see there are on open sessions to that specific path and will
     * call the handler.  In this case, the handler is responsible for
     * handling any checks or logic.
     *
     * @param[in] path - the blobId path.
     * @return bool - true if delete was successful.
     */
    bool deleteBlob(const std::string& path) override;

    /**
     * Attempt to write Metadata to a blob.
     *
     * @param[in] session - the session for this command.
     * @param[in] offset - the offset into the blob to write.
     * @param[in] data - the bytes to write to the blob.
     * @return bool - true if the write succeeded.
     */
    bool writeMeta(uint16_t session, uint32_t offset,
                   const std::vector<uint8_t>& data) override;

    /**
     * Attempts to return a valid unique session id.
     *
     * @param[in,out] - pointer to the session.
     * @return bool - true if able to allocate.
     */
    bool getSession(uint16_t* session);

  private:
    /**
     * Given a file path will return first handler to answer that it owns
     * it.
     *
     * @param[in] path - the file path.
     * @return pointer to the handler or nullptr if not found.
     */
    GenericBlobInterface* getHandler(const std::string& path);

    /**
     * Given a session id, update session time and return a handler to take
     * action
     *
     * @param[in] session - session ID
     * @param[in] requiredFlags - only return handler if the flags for this
     *            session contain these flags; defaults to any flag
     * @return session handler, nullptr if cannot get handler
     */
    GenericBlobInterface* getActionHandler(
        uint16_t session,
        uint16_t requiredFlags = std::numeric_limits<uint16_t>::max());

    /**
     * Helper method to erase a session from all maps
     *
     * @param[in] handler - handler pointer for lookup
     * @param[in] session - session ID for lookup
     * @return None
     */
    void eraseSession(GenericBlobInterface* const handler, uint16_t session);

    /**
     * For each session owned by this handler, call expire if it is stale
     *
     * @param[in] handler - handler pointer for lookup
     * @return None
     */
    void cleanUpStaleSessions(GenericBlobInterface* const handler);

    /* How long a session has to be inactive to be considered stale */
    std::chrono::seconds sessionTimeout;
    /* The next session ID to use */
    uint16_t next;
    /* Temporary list of blobIds used for enumeration. */
    std::vector<std::string> ids;
    /* List of Blob handler. */
    std::vector<std::unique_ptr<GenericBlobInterface>> handlers;
    /* Mapping of session ids to blob handlers and the path used with open.
     */
    std::unordered_map<uint16_t, SessionInfo> sessions;
    /* Mapping of open blobIds */
    std::unordered_map<std::string, int> openFiles;
    /* Map of handlers to their open sessions */
    std::unordered_map<GenericBlobInterface*, std::set<uint16_t>> openSessions;
};

/**
 * @brief Gets a handle to the BlobManager.
 *
 * @return a pointer to the BlobManager instance.
 */
ManagerInterface* getBlobManager();

} // namespace blobs
