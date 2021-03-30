#include "handler_unittest.hpp"

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::StartsWith;

using namespace std::string_literals;

namespace blobs
{

class BinaryStoreBlobHandlerStatTest : public BinaryStoreBlobHandlerTest
{
  protected:
    BinaryStoreBlobHandlerStatTest()
    {
        addDefaultStore(statTestBaseId);
    }

    static inline std::string statTestBaseId = "/test/"s;
    static inline std::string statTestBlobId = "/test/blob0"s;
    static inline std::vector<uint8_t> statTestData = {0, 1, 2, 3};
    static inline std::vector<uint8_t> statTestDataToOverwrite = {
        4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    static inline std::vector<uint8_t> commitMetaUnused;

    static inline uint16_t statTestSessionId = 0;
    static inline uint16_t statTestNewSessionId = 1;
    static inline uint32_t statTestOffset = 0;

    void openAndWriteTestData(
        const std::vector<uint8_t>& testData = statTestData)
    {
        uint16_t flags = OpenFlags::read | OpenFlags::write;
        EXPECT_TRUE(handler.open(statTestSessionId, flags, statTestBlobId));
        EXPECT_TRUE(handler.write(statTestSessionId, statTestOffset, testData));
    }

    void commitData()
    {
        EXPECT_TRUE(handler.commit(statTestSessionId, commitMetaUnused));
    }
};

TEST_F(BinaryStoreBlobHandlerStatTest, InitialStatIsValidQueriedWithBlobId)
{
    BlobMeta meta;

    /* Querying stat fails if there is no open session */
    EXPECT_FALSE(handler.stat(statTestSessionId, &meta));
    /* However stat completes if queried using blobId. Returning default. */
    EXPECT_TRUE(handler.stat(statTestBlobId, &meta));
    EXPECT_EQ(meta.size, 0);
};

TEST_F(BinaryStoreBlobHandlerStatTest, StatShowsCommittedState)
{
    BlobMeta meta;
    const int testIter = 2;

    openAndWriteTestData();
    for (int i = 0; i < testIter; ++i)
    {
        EXPECT_TRUE(handler.stat(statTestSessionId, &meta));
        EXPECT_EQ(meta.size, statTestData.size());
        EXPECT_TRUE(meta.blobState & OpenFlags::read);
        EXPECT_TRUE(meta.blobState & OpenFlags::write);
        EXPECT_FALSE(meta.blobState & StateFlags::committed);
        EXPECT_TRUE(meta.blobState & BinaryStore::CommitState::Dirty);
    }

    commitData();
    for (int i = 0; i < testIter; ++i)
    {
        EXPECT_TRUE(handler.stat(statTestSessionId, &meta));
        EXPECT_EQ(meta.size, statTestData.size());
        EXPECT_TRUE(meta.blobState & OpenFlags::read);
        EXPECT_TRUE(meta.blobState & OpenFlags::write);
        EXPECT_TRUE(meta.blobState & StateFlags::committed);
        EXPECT_TRUE(meta.blobState & BinaryStore::CommitState::Clean);
    }
}

TEST_F(BinaryStoreBlobHandlerStatTest, StatChangedWhenOverwriting)
{
    BlobMeta meta;
    const int testIter = 2;

    openAndWriteTestData();
    commitData();
    // Overwrite with different data.
    EXPECT_TRUE(handler.write(statTestSessionId, statTestOffset,
                              statTestDataToOverwrite));
    for (int i = 0; i < testIter; ++i)
    {
        EXPECT_TRUE(handler.stat(statTestSessionId, &meta));
        EXPECT_EQ(meta.size, statTestDataToOverwrite.size());
        EXPECT_TRUE(meta.blobState & OpenFlags::read);
        EXPECT_TRUE(meta.blobState & OpenFlags::write);
        EXPECT_FALSE(meta.blobState & StateFlags::committed);
        EXPECT_TRUE(meta.blobState & BinaryStore::CommitState::Dirty);
    }

    commitData();
    for (int i = 0; i < testIter; ++i)
    {
        EXPECT_TRUE(handler.stat(statTestSessionId, &meta));
        EXPECT_EQ(meta.size, statTestDataToOverwrite.size());
        EXPECT_TRUE(meta.blobState & OpenFlags::read);
        EXPECT_TRUE(meta.blobState & OpenFlags::write);
        EXPECT_TRUE(meta.blobState & StateFlags::committed);
        EXPECT_TRUE(meta.blobState & BinaryStore::CommitState::Clean);
    }
}

} // namespace blobs
