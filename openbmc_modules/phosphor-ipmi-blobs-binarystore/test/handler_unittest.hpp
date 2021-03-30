#pragma once

#include "binarystore_mock.hpp"
#include "fake_sys_file.hpp"
#include "handler.hpp"

#include <memory>
#include <string>

#include "binaryblob.pb.h"

#include <gtest/gtest.h>

using ::testing::Contains;

using namespace std::string_literals;
using namespace binstore;

namespace blobs
{

class BinaryStoreBlobHandlerTest : public ::testing::Test
{
  protected:
    BinaryStoreBlobHandlerTest() = default;

    std::unique_ptr<MockBinaryStore> defaultMockStore(const std::string& baseId)
    {
        return std::make_unique<MockBinaryStore>(
            baseId, std::make_unique<FakeSysFile>());
    }

    void addDefaultStore(const std::string& baseId)
    {
        handler.addNewBinaryStore(defaultMockStore(baseId));
    }

    BinaryStoreBlobHandler handler;
};

} // namespace blobs
