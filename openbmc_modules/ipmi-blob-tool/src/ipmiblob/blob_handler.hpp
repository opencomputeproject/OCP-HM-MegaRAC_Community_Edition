#pragma once

#include "blob_interface.hpp"
#include "ipmi_interface.hpp"

#include <memory>

namespace ipmiblob
{

class BlobHandler : public BlobInterface
{
  public:
    /**
     * Create a BlobInterface pointer for use given an ipmi handler.
     *
     * @note This is a convenience method.
     * @return a BlobHandler wrapped as a BlobInterface pointer.
     */
    static std::unique_ptr<BlobInterface>
        CreateBlobHandler(std::unique_ptr<IpmiInterface> ipmi);

    explicit BlobHandler(std::unique_ptr<IpmiInterface> ipmi) :
        ipmi(std::move(ipmi)){};

    ~BlobHandler() = default;
    BlobHandler(const BlobHandler&) = delete;
    BlobHandler& operator=(const BlobHandler&) = delete;
    BlobHandler(BlobHandler&&) = default;
    BlobHandler& operator=(BlobHandler&&) = default;

    /**
     * Retrieve the blob count.
     *
     * @return the number of blob_ids found (0 on failure).
     */
    int getBlobCount();

    /**
     * Given an index into the list of blobs, return the name.
     *
     * @param[in] index - the index into the list of blob ids.
     * @return the name as a string or empty on failure.
     */
    std::string enumerateBlob(std::uint32_t index);

    /**
     * @throws BlobException.
     */
    void commit(std::uint16_t session,
                const std::vector<std::uint8_t>& bytes = {}) override;

    /**
     * @throws BlobException.
     */
    void writeMeta(std::uint16_t session, std::uint32_t offset,
                   const std::vector<std::uint8_t>& bytes) override;

    /**
     * @throw BlobException.
     */
    void writeBytes(std::uint16_t session, std::uint32_t offset,
                    const std::vector<std::uint8_t>& bytes) override;

    std::vector<std::string> getBlobList() override;

    /**
     * @throws BlobException.
     */
    StatResponse getStat(const std::string& id) override;

    /**
     * @throws BlobException.
     */
    StatResponse getStat(std::uint16_t session) override;

    /**
     * @throws BlobException.
     */
    std::uint16_t openBlob(const std::string& id,
                           std::uint16_t handlerFlags) override;

    void closeBlob(std::uint16_t session) override;

    void deleteBlob(const std::string& id) override;

    /**
     * @throws BlobException.
     */
    std::vector<std::uint8_t> readBytes(std::uint16_t session,
                                        std::uint32_t offset,
                                        std::uint32_t length) override;

  private:
    /**
     * Send the contents of the payload to IPMI, this method handles wrapping
     * with the OEN, subcommand and CRC.
     *
     * @param[in] command - the blob command.
     * @param[in] payload - the payload bytes.
     * @return the bytes returned from the ipmi interface.
     * @throws BlobException.
     */
    std::vector<std::uint8_t>
        sendIpmiPayload(BlobOEMCommands command,
                        const std::vector<std::uint8_t>& payload);

    /**
     * Generic blob byte writer.
     *
     * @param[in] command - the command associated with this write.
     * @param[in] session - the session id.
     * @param[in] offset - the offset for the metadata to write.
     * @param[in] bytes - the bytes to send.
     * @throws BlobException on failure.
     */
    void writeGeneric(BlobOEMCommands command, std::uint16_t session,
                      std::uint32_t offset,
                      const std::vector<std::uint8_t>& bytes);

    /**
     * Generic stat reader.
     *
     * @param[in] command - the command associated with this write.
     * @param[in] request - the bytes of the request
     * @return the metadata StatResponse
     * @throws BlobException on failure.
     */
    StatResponse statGeneric(BlobOEMCommands command,
                             const std::vector<std::uint8_t>& request);

    std::unique_ptr<IpmiInterface> ipmi;
};

constexpr int ipmiOEMNetFn = 46;
constexpr int ipmiOEMBlobCmd = 128;

} // namespace ipmiblob
