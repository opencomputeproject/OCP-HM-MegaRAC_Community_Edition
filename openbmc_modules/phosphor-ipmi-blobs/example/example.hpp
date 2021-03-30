#pragma once

#include <blobs-ipmid/blobs.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This method must be declared as extern C for blob manager to lookup the
 * symbol.
 */
std::unique_ptr<blobs::GenericBlobInterface> createHandler();

#ifdef __cplusplus
}
#endif

namespace blobs
{

constexpr int kBufferSize = 1024;

struct ExampleBlob
{
    ExampleBlob() = default;
    ExampleBlob(uint16_t id, uint16_t flags) :
        sessionId(id), flags(flags),
        state(StateFlags::open_read | StateFlags::open_write), length(0)
    {
    }

    /* The blob handler session id. */
    uint16_t sessionId;

    /* The flags passed into open. */
    uint16_t flags;

    /* The current state. */
    uint16_t state;

    /* The buffer is a fixed size, but length represents the number of bytes
     * expected to be used contiguously from offset 0.
     */
    uint32_t length;

    /* The staging buffer. */
    uint8_t buffer[kBufferSize];
};

class ExampleBlobHandler : public GenericBlobInterface
{
  public:
    /* We want everything explicitly default. */
    ExampleBlobHandler() = default;
    ~ExampleBlobHandler() = default;
    ExampleBlobHandler(const ExampleBlobHandler&) = default;
    ExampleBlobHandler& operator=(const ExampleBlobHandler&) = default;
    ExampleBlobHandler(ExampleBlobHandler&&) = default;
    ExampleBlobHandler& operator=(ExampleBlobHandler&&) = default;

    bool canHandleBlob(const std::string& path) override;
    std::vector<std::string> getBlobIds() override;
    bool deleteBlob(const std::string& path) override;
    bool stat(const std::string& path, BlobMeta* meta) override;
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
    bool stat(uint16_t session, BlobMeta* meta) override;
    bool expire(uint16_t session) override;

    constexpr static char supportedPath[] = "/dev/fake/command";

  private:
    ExampleBlob* getSession(uint16_t id);

    std::unordered_map<uint16_t, ExampleBlob> sessions;
};

} // namespace blobs
