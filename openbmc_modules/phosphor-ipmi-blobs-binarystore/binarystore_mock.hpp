#pragma once

#include "binarystore.hpp"
#include "binarystore_interface.hpp"
#include "sys_file.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>

using ::testing::Invoke;

namespace binstore
{

class MockBinaryStore : public BinaryStoreInterface
{
  public:
    MockBinaryStore(const std::string& baseBlobId,
                    std::unique_ptr<SysFile> file) :
        real_store_(baseBlobId, std::move(file))
    {
        // Implemented calls in BinaryStore will be directed to the real object.
        ON_CALL(*this, getBaseBlobId)
            .WillByDefault(Invoke(&real_store_, &BinaryStore::getBaseBlobId));
        ON_CALL(*this, getBlobIds)
            .WillByDefault(Invoke(&real_store_, &BinaryStore::getBlobIds));
        ON_CALL(*this, openOrCreateBlob)
            .WillByDefault(
                Invoke(&real_store_, &BinaryStore::openOrCreateBlob));
        ON_CALL(*this, close)
            .WillByDefault(Invoke(&real_store_, &BinaryStore::close));
        ON_CALL(*this, read)
            .WillByDefault(Invoke(&real_store_, &BinaryStore::read));
        ON_CALL(*this, write)
            .WillByDefault(Invoke(&real_store_, &BinaryStore::write));
        ON_CALL(*this, commit)
            .WillByDefault(Invoke(&real_store_, &BinaryStore::commit));
        ON_CALL(*this, stat)
            .WillByDefault(Invoke(&real_store_, &BinaryStore::stat));
    }
    MOCK_CONST_METHOD0(getBaseBlobId, std::string());
    MOCK_CONST_METHOD0(getBlobIds, std::vector<std::string>());
    MOCK_METHOD2(openOrCreateBlob, bool(const std::string&, uint16_t));
    MOCK_METHOD1(deleteBlob, bool(const std::string&));
    MOCK_METHOD2(read, std::vector<uint8_t>(uint32_t, uint32_t));
    MOCK_METHOD2(write, bool(uint32_t, const std::vector<uint8_t>&));
    MOCK_METHOD0(commit, bool());
    MOCK_METHOD0(close, bool());
    MOCK_METHOD1(stat, bool(blobs::BlobMeta* meta));

  private:
    BinaryStore real_store_;
};

} // namespace binstore
