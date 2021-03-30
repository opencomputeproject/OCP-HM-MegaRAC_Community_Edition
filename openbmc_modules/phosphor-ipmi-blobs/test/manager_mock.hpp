#pragma once

#include "manager.hpp"

#include <blobs-ipmid/blobs.hpp>
#include <memory>
#include <string>

#include <gmock/gmock.h>

namespace blobs
{

class ManagerMock : public ManagerInterface
{
  public:
    virtual ~ManagerMock() = default;

    MOCK_METHOD1(registerHandler, bool(std::unique_ptr<GenericBlobInterface>));
    MOCK_METHOD0(buildBlobList, uint32_t());
    MOCK_METHOD1(getBlobId, std::string(uint32_t));
    MOCK_METHOD3(open, bool(uint16_t, const std::string&, uint16_t*));
    MOCK_METHOD2(stat, bool(const std::string&, BlobMeta*));
    MOCK_METHOD2(stat, bool(uint16_t, BlobMeta*));
    MOCK_METHOD2(commit, bool(uint16_t, const std::vector<uint8_t>&));
    MOCK_METHOD1(close, bool(uint16_t));
    MOCK_METHOD3(read, std::vector<uint8_t>(uint16_t, uint32_t, uint32_t));
    MOCK_METHOD3(write, bool(uint16_t, uint32_t, const std::vector<uint8_t>&));
    MOCK_METHOD1(deleteBlob, bool(const std::string&));
    MOCK_METHOD3(writeMeta,
                 bool(uint16_t, uint32_t, const std::vector<uint8_t>&));
};

/*
 * Provide a one-off implementation for now.
 * To test this, set managerReturned as you need.
 */
ManagerInterface* managerReturned;

ManagerInterface* getBlobManager()
{
    return managerReturned;
}

} // namespace blobs
