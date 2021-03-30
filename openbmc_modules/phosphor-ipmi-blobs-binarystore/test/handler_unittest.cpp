#include "handler_unittest.hpp"

#include "fake_sys_file.hpp"

#include <google/protobuf/message_lite.h>
#include <google/protobuf/text_format.h>

#include <algorithm>
#include <boost/endian/arithmetic.hpp>
#include <memory>
#include <string>
#include <vector>

#include "binaryblob.pb.h"

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::ElementsAreArray;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrNe;
using ::testing::UnorderedElementsAreArray;

using namespace std::string_literals;
using namespace binstore;

namespace blobs
{

class BinaryStoreBlobHandlerBasicTest : public BinaryStoreBlobHandlerTest
{
  protected:
    static inline std::string basicTestBaseId = "/test/"s;
    static inline std::string basicTestBlobId = "/test/blob0"s;

    static const std::vector<std::string> basicTestBaseIdList;
    static inline std::string basicTestInvalidBlobId = "/invalid/blob0"s;

    void addAllBaseIds()
    {
        for (size_t i = 0; i < basicTestBaseIdList.size(); ++i)
        {
            auto store = defaultMockStore(basicTestBaseIdList[i]);

            EXPECT_CALL(*store, getBaseBlobId()).Times(AtLeast(1));

            handler.addNewBinaryStore(std::move(store));
        }
    }
};

const std::vector<std::string>
    BinaryStoreBlobHandlerBasicTest::basicTestBaseIdList = {
        BinaryStoreBlobHandlerBasicTest::basicTestBaseId, "/another/"s,
        "/null\0/"s};

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobZeroStoreFail)
{
    // Cannot handle since there is no store. Shouldn't crash.
    EXPECT_FALSE(handler.canHandleBlob(basicTestInvalidBlobId));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobChecksName)
{
    auto store = defaultMockStore(basicTestBaseId);

    EXPECT_CALL(*store, getBaseBlobId()).Times(AtLeast(1));

    handler.addNewBinaryStore(std::move(store));

    // Verify canHandleBlob checks and returns false on an invalid name.
    EXPECT_FALSE(handler.canHandleBlob(basicTestInvalidBlobId));
    // Verify canHandleBlob return true for a blob id that it can handle
    EXPECT_TRUE(handler.canHandleBlob(basicTestBlobId));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, GetBlobIdEqualsConcatenationsOfBaseIds)
{
    addAllBaseIds();

    // When there is no other blob id, ids are just base ids (might be
    // re-ordered).
    EXPECT_THAT(handler.getBlobIds(),
                UnorderedElementsAreArray(basicTestBaseIdList));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobInvalidNames)
{
    addAllBaseIds();

    const std::vector<std::string> invalidNames = {
        basicTestInvalidBlobId,
        "/"s,
        "//"s,
        "/test"s, // Cannot handle the base path
        "/test/"s,
        "/test/this/blob"s, // Cannot handle nested path
    };

    // Unary helper for algorithm
    auto canHandle = [this](const std::string& blobId) {
        return handler.canHandleBlob(blobId);
    };

    EXPECT_TRUE(
        std::none_of(invalidNames.cbegin(), invalidNames.cend(), canHandle));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, CanHandleBlobValidNames)
{
    addAllBaseIds();

    const std::vector<std::string> validNames = {
        basicTestBlobId,  "/test/test"s,          "/test/xyz.abc"s,
        "/another/blob"s, "/null\0/\0\0zer\0\0"s,
    };

    // Unary helper for algorithm
    auto canHandle = [this](const std::string& blobId) {
        return handler.canHandleBlob(blobId);
    };

    // Verify canHandleBlob can handle a valid blob under the path.
    EXPECT_TRUE(std::all_of(validNames.cbegin(), validNames.cend(), canHandle));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, DeleteReturnsWhatStoreReturns)
{
    auto store = defaultMockStore(basicTestBaseId);

    EXPECT_CALL(*store, getBaseBlobId()).Times(AtLeast(1));
    EXPECT_CALL(*store, deleteBlob(StrEq(basicTestBlobId)))
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    handler.addNewBinaryStore(std::move(store));

    // Unary helper for algorithm
    auto canHandle = [this](const std::string& blobId) {
        return handler.canHandleBlob(blobId);
    };

    // Verify canHandleBlob return true for a blob id that it can handle
    EXPECT_FALSE(canHandle(basicTestInvalidBlobId));
    EXPECT_TRUE(canHandle(basicTestBlobId));
    EXPECT_FALSE(handler.deleteBlob(basicTestInvalidBlobId));
    EXPECT_FALSE(handler.deleteBlob(basicTestBlobId));
    EXPECT_TRUE(handler.deleteBlob(basicTestBlobId));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, StaleDataIsClearedDuringCreation)
{
    using namespace google::protobuf;

    auto basicTestStaleBlobStr =
        R"(blob_base_id: "/stale/"
          blobs {
            blob_id: "/stale/blob"
          })";

    // Create sysfile containing a valid but stale blob
    const std::string staleBaseId = "/stale/"s;
    const std::string staleBlobId = "/stale/blob"s;
    binaryblobproto::BinaryBlobBase staleBlob;
    EXPECT_TRUE(TextFormat::ParseFromString(basicTestStaleBlobStr, &staleBlob));

    // Serialize to string stored in the fakeSysFile
    auto staleBlobData = staleBlob.SerializeAsString();
    boost::endian::little_uint64_t sizeLE = staleBlobData.size();
    std::string commitData(reinterpret_cast<const char*>(sizeLE.data()),
                           sizeof(sizeLE));
    commitData += staleBlobData;

    std::vector<std::string> expectedIdList = {basicTestBaseId};

    handler.addNewBinaryStore(BinaryStore::createFromConfig(
        basicTestBaseId, std::make_unique<FakeSysFile>(commitData)));
    EXPECT_FALSE(handler.canHandleBlob(staleBlobId));
    EXPECT_EQ(handler.getBlobIds(), expectedIdList);
}

TEST_F(BinaryStoreBlobHandlerBasicTest, CreatingFromEmptySysfile)
{
    const std::string emptyData;
    EXPECT_NO_THROW(handler.addNewBinaryStore(BinaryStore::createFromConfig(
        basicTestBaseId, std::make_unique<FakeSysFile>(emptyData))));
    EXPECT_TRUE(handler.canHandleBlob(basicTestBlobId));
}

TEST_F(BinaryStoreBlobHandlerBasicTest, CreatingFromJunkData)
{
    boost::endian::little_uint64_t tooLarge = 0xffffffffffffffffull;
    const std::string junkDataWithLargeSize(
        reinterpret_cast<const char*>(tooLarge.data()), sizeof(tooLarge));
    EXPECT_GE(tooLarge, junkDataWithLargeSize.max_size());

    EXPECT_NO_THROW(handler.addNewBinaryStore(BinaryStore::createFromConfig(
        basicTestBaseId,
        std::make_unique<FakeSysFile>(junkDataWithLargeSize))));

    EXPECT_TRUE(handler.canHandleBlob(basicTestBlobId));
}

} // namespace blobs
