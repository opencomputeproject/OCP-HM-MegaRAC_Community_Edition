#pragma once

#include <blobs-ipmid/blobs.hpp>

#include <gmock/gmock.h>

namespace blobs
{

class BlobMock : public GenericBlobInterface
{
  public:
    virtual ~BlobMock() = default;

    MOCK_METHOD1(canHandleBlob, bool(const std::string&));
    MOCK_METHOD0(getBlobIds, std::vector<std::string>());
    MOCK_METHOD1(deleteBlob, bool(const std::string&));
    MOCK_METHOD2(stat, bool(const std::string&, BlobMeta*));
    MOCK_METHOD3(open, bool(uint16_t, uint16_t, const std::string&));
    MOCK_METHOD3(read, std::vector<uint8_t>(uint16_t, uint32_t, uint32_t));
    MOCK_METHOD3(write, bool(uint16_t, uint32_t, const std::vector<uint8_t>&));
    MOCK_METHOD3(writeMeta,
                 bool(uint16_t, uint32_t, const std::vector<uint8_t>&));
    MOCK_METHOD2(commit, bool(uint16_t, const std::vector<uint8_t>&));
    MOCK_METHOD1(close, bool(uint16_t));
    MOCK_METHOD2(stat, bool(uint16_t, BlobMeta*));
    MOCK_METHOD1(expire, bool(uint16_t));
};
} // namespace blobs
