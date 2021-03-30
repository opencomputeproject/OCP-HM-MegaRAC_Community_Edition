#include "handler_unittest.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::StartsWith;

using namespace std::string_literals;

namespace blobs
{

class BinaryStoreBlobHandlerCommitTest : public BinaryStoreBlobHandlerTest
{
  protected:
    BinaryStoreBlobHandlerCommitTest()
    {
        addDefaultStore(commitTestBaseId);
    }

    static inline std::string commitTestBaseId = "/test/"s;
    static inline std::string commitTestBlobId = "/test/blob0"s;
    static inline std::vector<uint8_t> commitTestData = {0, 1, 2, 3, 4,
                                                         5, 6, 7, 8, 9};
    static inline std::vector<uint8_t> commitTestDataToOverwrite = {10, 11, 12,
                                                                    13, 14};
    static inline std::vector<uint8_t> commitMetaUnused;

    static inline uint16_t commitTestSessionId = 0;
    static inline uint16_t commitTestNewSessionId = 1;
    static inline uint32_t commitTestOffset = 0;

    void openAndWriteTestData()
    {
        uint16_t flags = OpenFlags::read | OpenFlags::write;
        EXPECT_TRUE(handler.open(commitTestSessionId, flags, commitTestBlobId));
        EXPECT_TRUE(handler.write(commitTestSessionId, commitTestOffset,
                                  commitTestData));
    }

    void openWriteThenCommitData()
    {
        openAndWriteTestData();
        EXPECT_TRUE(handler.commit(commitTestSessionId, commitMetaUnused));
    }
};

TEST_F(BinaryStoreBlobHandlerCommitTest, CommittedDataIsReadable)
{
    openWriteThenCommitData();

    EXPECT_EQ(commitTestData,
              handler.read(commitTestSessionId, commitTestOffset,
                           commitTestData.size()));
}

TEST_F(BinaryStoreBlobHandlerCommitTest, CommittedDataCanBeReopened)
{
    openWriteThenCommitData();

    EXPECT_TRUE(handler.close(commitTestSessionId));
    EXPECT_TRUE(handler.open(commitTestNewSessionId, OpenFlags::read,
                             commitTestBlobId));
    EXPECT_EQ(commitTestData,
              handler.read(commitTestNewSessionId, commitTestOffset,
                           commitTestData.size()));
}

TEST_F(BinaryStoreBlobHandlerCommitTest, OverwrittenDataCanBeCommitted)
{
    openWriteThenCommitData();

    EXPECT_TRUE(handler.write(commitTestSessionId, commitTestOffset,
                              commitTestDataToOverwrite));
    EXPECT_TRUE(handler.commit(commitTestSessionId, commitMetaUnused));
    EXPECT_TRUE(handler.close(commitTestSessionId));

    EXPECT_TRUE(handler.open(commitTestNewSessionId, OpenFlags::read,
                             commitTestBlobId));
    EXPECT_EQ(commitTestDataToOverwrite,
              handler.read(commitTestNewSessionId, commitTestOffset,
                           commitTestDataToOverwrite.size()));
}

TEST_F(BinaryStoreBlobHandlerCommitTest, UncommittedDataIsLostUponClose)
{
    openWriteThenCommitData();

    EXPECT_TRUE(handler.write(commitTestSessionId, commitTestOffset,
                              commitTestDataToOverwrite));
    EXPECT_TRUE(handler.close(commitTestSessionId));

    EXPECT_TRUE(handler.open(commitTestNewSessionId, OpenFlags::read,
                             commitTestBlobId));
    EXPECT_EQ(commitTestData,
              handler.read(commitTestNewSessionId, commitTestOffset,
                           commitTestData.size()));
}

} // namespace blobs
