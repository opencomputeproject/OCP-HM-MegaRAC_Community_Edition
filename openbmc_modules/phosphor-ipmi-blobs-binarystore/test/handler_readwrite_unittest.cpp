#include "handler_unittest.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::StartsWith;

using namespace std::string_literals;
using namespace binstore;

namespace blobs
{

class BinaryStoreBlobHandlerReadWriteTest : public BinaryStoreBlobHandlerTest
{
  protected:
    BinaryStoreBlobHandlerReadWriteTest()
    {
        addDefaultStore(rwTestBaseId);
    }

    static inline std::string rwTestBaseId = "/test/"s;
    static inline std::string rwTestBlobId = "/test/blob0"s;
    static inline std::vector<uint8_t> rwTestData = {0, 1, 2, 3, 4,
                                                     5, 6, 7, 8, 9};
    static inline uint16_t rwTestROFlags = OpenFlags::read;
    static inline uint16_t rwTestRWFlags = OpenFlags::read | OpenFlags::write;
    static inline uint16_t rwTestSessionId = 0;
    static inline uint32_t rwTestOffset = 0;

    void openAndWriteTestData()
    {
        EXPECT_TRUE(handler.open(rwTestSessionId, rwTestRWFlags, rwTestBlobId));
        EXPECT_TRUE(handler.write(rwTestSessionId, rwTestOffset, rwTestData));
    }
};

TEST_F(BinaryStoreBlobHandlerReadWriteTest, ReadWriteReturnsWhatStoreReturns)
{
    const std::vector<uint8_t> emptyData;
    auto store = defaultMockStore(rwTestBaseId);

    EXPECT_CALL(*store, openOrCreateBlob(_, rwTestRWFlags))
        .WillOnce(Return(true));
    EXPECT_CALL(*store, read(rwTestOffset, _))
        .WillOnce(Return(emptyData))
        .WillOnce(Return(rwTestData));

    EXPECT_CALL(*store, write(rwTestOffset, emptyData)).WillOnce(Return(false));
    EXPECT_CALL(*store, write(rwTestOffset, rwTestData)).WillOnce(Return(true));

    handler.addNewBinaryStore(std::move(store));

    EXPECT_TRUE(handler.open(rwTestSessionId, rwTestRWFlags, rwTestBlobId));
    EXPECT_EQ(emptyData, handler.read(rwTestSessionId, rwTestOffset, 1));
    EXPECT_EQ(rwTestData, handler.read(rwTestSessionId, rwTestOffset, 1));
    EXPECT_FALSE(handler.write(rwTestSessionId, rwTestOffset, emptyData));
    EXPECT_TRUE(handler.write(rwTestSessionId, rwTestOffset, rwTestData));
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, WriteFailForWrongSession)
{
    uint16_t wrongSessionId = 1;
    EXPECT_TRUE(handler.open(rwTestSessionId, rwTestRWFlags, rwTestBlobId));
    EXPECT_FALSE(handler.write(wrongSessionId, rwTestOffset, rwTestData));
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, WriteFailForWrongFlag)
{
    EXPECT_TRUE(handler.open(rwTestSessionId, rwTestROFlags, rwTestBlobId));
    EXPECT_FALSE(handler.write(rwTestSessionId, rwTestOffset, rwTestData));
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, ReadReturnEmptyForWrongSession)
{
    uint16_t wrongSessionId = 1;
    EXPECT_TRUE(handler.open(rwTestSessionId, rwTestROFlags, rwTestBlobId));
    EXPECT_THAT(handler.read(wrongSessionId, rwTestOffset, rwTestData.size()),
                IsEmpty());
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, AbleToReadDataWritten)
{
    openAndWriteTestData();
    EXPECT_EQ(rwTestData,
              handler.read(rwTestSessionId, rwTestOffset, rwTestData.size()));
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, ReadBeyondEndReturnsEmpty)
{
    openAndWriteTestData();
    const size_t offsetBeyondEnd = rwTestData.size();
    EXPECT_THAT(
        handler.read(rwTestSessionId, offsetBeyondEnd, rwTestData.size()),
        IsEmpty());
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, ReadAtOffset)
{
    openAndWriteTestData();
    EXPECT_EQ(rwTestData,
              handler.read(rwTestSessionId, rwTestOffset, rwTestData.size()));

    // Now read over the entire offset range for 1 byte
    for (size_t i = 0; i < rwTestData.size(); ++i)
    {
        EXPECT_THAT(handler.read(rwTestSessionId, i, 1),
                    ElementsAre(rwTestData[i]));
    }
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, PartialOverwriteDataWorks)
{
    // Construct a partially overwritten vector
    const size_t overwritePos = 8;
    std::vector<uint8_t> overwriteData = {8, 9, 10, 11, 12};
    std::vector<uint8_t> expectedOverWrittenData(
        rwTestData.begin(), rwTestData.begin() + overwritePos);
    expectedOverWrittenData.insert(expectedOverWrittenData.end(),
                                   overwriteData.begin(), overwriteData.end());

    openAndWriteTestData();
    EXPECT_EQ(rwTestData,
              handler.read(rwTestSessionId, rwTestOffset, rwTestData.size()));
    EXPECT_TRUE(handler.write(rwTestSessionId, overwritePos, overwriteData));

    EXPECT_EQ(expectedOverWrittenData,
              handler.read(rwTestSessionId, 0, expectedOverWrittenData.size()));

    EXPECT_THAT(
        handler.read(rwTestSessionId, expectedOverWrittenData.size(), 1),
        IsEmpty());
}

TEST_F(BinaryStoreBlobHandlerReadWriteTest, WriteWithGapShouldFail)
{
    const auto gapOffset = 10;
    EXPECT_TRUE(handler.open(rwTestSessionId, rwTestRWFlags, rwTestBlobId));
    EXPECT_FALSE(handler.write(rwTestSessionId, gapOffset, rwTestData));
}

} // namespace blobs
