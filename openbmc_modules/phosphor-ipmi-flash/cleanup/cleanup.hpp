#pragma once

#include "fs.hpp"

#include <blobs-ipmid/blobs.hpp>

#include <memory>
#include <string>
#include <vector>

namespace ipmi_flash
{

class FileCleanupHandler : public blobs::GenericBlobInterface
{
  public:
    static std::unique_ptr<blobs::GenericBlobInterface>
        CreateCleanupHandler(const std::string& blobId,
                             const std::vector<std::string>& files,
                             std::unique_ptr<FileSystemInterface> helper);

    FileCleanupHandler(const std::string& blobId,
                       const std::vector<std::string>& files,
                       std::unique_ptr<FileSystemInterface> helper) :
        supported(blobId),
        files(files), helper(std::move(helper))
    {}

    ~FileCleanupHandler() = default;
    FileCleanupHandler(const FileCleanupHandler&) = default;
    FileCleanupHandler& operator=(const FileCleanupHandler&) = default;
    FileCleanupHandler(FileCleanupHandler&&) = default;
    FileCleanupHandler& operator=(FileCleanupHandler&&) = default;

    bool canHandleBlob(const std::string& path) override;
    std::vector<std::string> getBlobIds() override;
    bool commit(uint16_t session, const std::vector<uint8_t>& data) override;

    /* These methods return true without doing anything. */
    bool open(uint16_t session, uint16_t flags,
              const std::string& path) override
    {
        return true;
    }
    bool close(uint16_t session) override
    {
        return true;
    }
    bool expire(uint16_t session) override
    {
        return true;
    }

    /* These methods are unsupported. */
    bool deleteBlob(const std::string& path) override
    {
        return false;
    }
    bool stat(const std::string& path, blobs::BlobMeta* meta) override
    {
        return false;
    }
    std::vector<uint8_t> read(uint16_t session, uint32_t offset,
                              uint32_t requestedSize) override
    {
        return {};
    }
    bool write(uint16_t session, uint32_t offset,
               const std::vector<uint8_t>& data) override
    {
        return false;
    }
    bool writeMeta(uint16_t session, uint32_t offset,
                   const std::vector<uint8_t>& data) override
    {
        return false;
    }
    bool stat(uint16_t session, blobs::BlobMeta* meta) override
    {
        return false;
    }

  private:
    std::string supported;
    std::vector<std::string> files;
    std::unique_ptr<FileSystemInterface> helper;
};

} // namespace ipmi_flash
