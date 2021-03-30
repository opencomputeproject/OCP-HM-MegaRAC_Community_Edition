#include "sys_file_impl.hpp"
#include "sys_mock.hpp"

#include <fcntl.h>

#include <cstring>
#include <memory>

#include <gmock/gmock.h>

using namespace binstore;
using namespace std::string_literals;

using ::testing::_;
using ::testing::Args;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetErrnoAndReturn;
using ::testing::StrEq;
using ::testing::WithArgs;

static constexpr int sysFileTestFd = 42;
static inline std::string sysFileTestPath = "/test/path"s;
static constexpr size_t sysFileTestOffset = 0;
auto static const sysFileTestStr = "Hello, \0+.world!\0"s;
std::vector<uint8_t> static const sysFileTestBuf(sysFileTestStr.begin(),
                                                 sysFileTestStr.end());

class SysFileTest : public ::testing::Test
{
  protected:
    SysFileTest()
    {
        EXPECT_CALL(sys, open(StrEq(sysFileTestPath), O_RDWR))
            .WillOnce(Return(sysFileTestFd));
        EXPECT_CALL(sys, close(sysFileTestFd));

        file = std::make_unique<SysFileImpl>(sysFileTestPath, sysFileTestOffset,
                                             &sys);
    }

    const internal::SysMock sys;

    std::unique_ptr<SysFile> file;
};

ACTION_P(BufSet, buf)
{
    size_t count = std::min(arg1, buf.size());
    std::memcpy(arg0, buf.data(), count);

    return count;
}

ACTION_P2(BufSetAndGetStartAddr, buf, addrPtr)
{
    size_t count = std::min(arg1, buf.size());
    std::memcpy(arg0, buf.data(), count);
    *addrPtr = arg0;

    return count;
}

ACTION_P2(BufSetTruncated, buf, offset)
{
    size_t count = std::min(arg1, buf.size() - offset);
    std::memcpy(arg0, buf.data() + offset, count);

    return count;
}

TEST_F(SysFileTest, ReadSucceeds)
{
    EXPECT_CALL(sys, lseek(sysFileTestFd, 0, SEEK_SET));
    EXPECT_CALL(sys, read(sysFileTestFd, NotNull(), _))
        .WillOnce(WithArgs<1, 2>(BufSet(sysFileTestBuf)));

    EXPECT_EQ(sysFileTestStr, file->readAsStr(0, sysFileTestBuf.size()));
}

TEST_F(SysFileTest, ReadMoreThanAvailable)
{
    EXPECT_CALL(sys, lseek(sysFileTestFd, 0, SEEK_SET));
    EXPECT_CALL(sys, read(sysFileTestFd, NotNull(), _))
        .WillOnce(WithArgs<1, 2>(BufSet(sysFileTestBuf)))
        .WillOnce(Return(0));

    EXPECT_EQ(sysFileTestStr, file->readAsStr(0, sysFileTestBuf.size() + 1024));
}

TEST_F(SysFileTest, ReadAtOffset)
{
    const size_t testOffset = 2;
    std::string truncBuf = sysFileTestStr.substr(testOffset);

    EXPECT_CALL(sys, lseek(sysFileTestFd, testOffset, SEEK_SET));
    EXPECT_CALL(sys, read(sysFileTestFd, NotNull(), _))
        .WillOnce(WithArgs<1, 2>(BufSetTruncated(sysFileTestBuf, testOffset)));

    EXPECT_EQ(truncBuf, file->readAsStr(testOffset, truncBuf.size()));
}

TEST_F(SysFileTest, ReadRemainingFail)
{
    EXPECT_CALL(sys, lseek(sysFileTestFd, 0, SEEK_SET));
    EXPECT_CALL(sys, read(sysFileTestFd, NotNull(), _))
        .WillOnce(SetErrnoAndReturn(EIO, -1));

    EXPECT_THROW(file->readRemainingAsStr(0), std::exception);
}

TEST_F(SysFileTest, ReadRemainingSucceeds)
{
    EXPECT_CALL(sys, lseek(sysFileTestFd, 0, SEEK_SET));
    EXPECT_CALL(sys, read(sysFileTestFd, NotNull(), _))
        .WillOnce(WithArgs<1, 2>(BufSet(sysFileTestBuf)))
        .WillOnce(Return(0)); // EOF

    EXPECT_EQ(sysFileTestStr, file->readRemainingAsStr(0));
}

TEST_F(SysFileTest, ReadRemainingBeyondEndReturnsEmpty)
{
    const size_t largeOffset = 9000;
    EXPECT_CALL(sys, lseek(sysFileTestFd, largeOffset, SEEK_SET));
    EXPECT_CALL(sys, read(sysFileTestFd, NotNull(), _)).WillOnce(Return(0));

    EXPECT_THAT(file->readRemainingAsStr(largeOffset), IsEmpty());
}
