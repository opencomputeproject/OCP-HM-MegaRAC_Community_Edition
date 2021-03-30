#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gpioplus/internal/fd.hpp>
#include <gpioplus/test/sys.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <signal.h>
#include <sys/prctl.h>
#include <system_error>
#include <type_traits>
#include <utility>

#ifdef HAVE_GCOV
// Needed for the abrt test
extern "C" void __gcov_flush(void);
#endif

namespace gpioplus
{
namespace internal
{
namespace
{

using testing::Assign;
using testing::DoAll;
using testing::Return;

class FdTest : public testing::Test
{
  protected:
    const int expected_fd = 1234;
    const int expected_fd2 = 2345;
    const int expected_fd3 = 3456;
    testing::StrictMock<test::SysMock> mock;
    testing::StrictMock<test::SysMock> mock2;
};

TEST_F(FdTest, ConstructSimple)
{
    Fd fd(expected_fd, std::false_type(), &mock);
    EXPECT_EQ(expected_fd, *fd);
    EXPECT_EQ(&mock, fd.getSys());

    EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
}

TEST_F(FdTest, ConstructSimplBadFd)
{
    Fd fd(-1, std::false_type(), &mock);
    EXPECT_EQ(-1, *fd);
}

TEST_F(FdTest, ConstructDup)
{
    EXPECT_CALL(mock, dup(expected_fd)).WillOnce(Return(expected_fd2));
    Fd fd(expected_fd, &mock);
    EXPECT_EQ(expected_fd2, *fd);
    EXPECT_EQ(&mock, fd.getSys());

    EXPECT_CALL(mock, close(expected_fd2)).WillOnce(Return(0));
}

TEST_F(FdTest, ConstructDupFail)
{
    EXPECT_CALL(mock, dup(expected_fd))
        .WillOnce(DoAll(Assign(&errno, EINVAL), Return(-1)));
    EXPECT_THROW(Fd(expected_fd, &mock), std::system_error);
}

void abrt_handler(int signum)
{
    if (signum == SIGABRT)
    {
#ifdef HAVE_GCOV
        __gcov_flush();
#endif
    }
}

TEST_F(FdTest, CloseFails)
{
    EXPECT_DEATH(
        {
            struct sigaction act;
            act.sa_handler = abrt_handler;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;
            ASSERT_EQ(0, sigaction(SIGABRT, &act, nullptr));
            ASSERT_EQ(0, prctl(PR_SET_DUMPABLE, 0, 0, 0, 0));
            EXPECT_CALL(mock, close(expected_fd))
                .WillOnce(DoAll(Assign(&errno, EINVAL), Return(-1)));
            Fd(expected_fd, std::false_type(), &mock);
        },
        "");
}

TEST_F(FdTest, ConstructSuccess)
{
    const char* path = "/no-such-path/gpio";
    const int flags = O_RDWR;
    EXPECT_CALL(mock, open(path, flags)).WillOnce(Return(expected_fd));
    Fd fd(path, flags, &mock);
    EXPECT_EQ(expected_fd, *fd);
    EXPECT_EQ(&mock, fd.getSys());

    EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
}

TEST_F(FdTest, ConstructError)
{
    const char* path = "/no-such-path/gpio";
    const int flags = O_RDWR;
    EXPECT_CALL(mock, open(path, flags))
        .WillOnce(DoAll(Assign(&errno, EBUSY), Return(-1)));
    EXPECT_THROW(Fd(path, flags, &mock), std::system_error);
}

TEST_F(FdTest, ConstructCopy)
{
    Fd fd(expected_fd, std::false_type(), &mock);
    {
        EXPECT_CALL(mock, dup(expected_fd)).WillOnce(Return(expected_fd2));
        Fd fd2(fd);
        EXPECT_EQ(expected_fd2, *fd2);
        EXPECT_EQ(expected_fd, *fd);

        EXPECT_CALL(mock, close(expected_fd2)).WillOnce(Return(0));
    }

    EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
}

TEST_F(FdTest, OperatorCopySame)
{
    Fd fd(expected_fd, std::false_type(), &mock);
    fd = fd;
    EXPECT_EQ(expected_fd, *fd);

    EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
}

TEST_F(FdTest, OperatorCopy)
{
    Fd fd(expected_fd, std::false_type(), &mock);
    {
        Fd fd2(expected_fd2, std::false_type(), &mock2);
        EXPECT_CALL(mock2, close(expected_fd2)).WillOnce(Return(0));
        EXPECT_CALL(mock, dup(expected_fd)).WillOnce(Return(expected_fd3));
        fd2 = fd;
        EXPECT_EQ(expected_fd3, *fd2);
        EXPECT_EQ(&mock, fd2.getSys());
        EXPECT_EQ(expected_fd, *fd);
        EXPECT_EQ(&mock, fd.getSys());

        EXPECT_CALL(mock, close(expected_fd3)).WillOnce(Return(0));
    }

    EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
}

TEST_F(FdTest, ConstructMove)
{
    Fd fd(expected_fd, std::false_type(), &mock);
    {
        Fd fd2(std::move(fd));
        EXPECT_EQ(expected_fd, *fd2);
        EXPECT_EQ(-1, *fd);

        EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
    }
}

TEST_F(FdTest, OperatorMoveSame)
{
    Fd fd(expected_fd, std::false_type(), &mock);
    fd = std::move(fd);
    EXPECT_EQ(expected_fd, *fd);

    EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
}

TEST_F(FdTest, OperatorMove)
{
    Fd fd(expected_fd, std::false_type(), &mock);
    {
        Fd fd2(expected_fd2, std::false_type(), &mock2);
        EXPECT_CALL(mock2, close(expected_fd2)).WillOnce(Return(0));
        fd2 = std::move(fd);
        EXPECT_EQ(expected_fd, *fd2);
        EXPECT_EQ(&mock, fd2.getSys());
        EXPECT_EQ(-1, *fd);
        EXPECT_EQ(&mock, fd.getSys());

        EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
    }
}

class FdMethodTest : public FdTest
{
  protected:
    const int flags_blocking = O_SYNC | O_NOATIME;
    const int flags_noblocking = O_NONBLOCK | flags_blocking;
    std::unique_ptr<Fd> fd;

    void SetUp()
    {
        fd = std::make_unique<Fd>(expected_fd, std::false_type(), &mock);
    }

    void TearDown()
    {
        EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
        fd.reset();
    }
};

TEST_F(FdMethodTest, SetBlockingOnBlocking)
{
    EXPECT_CALL(mock, fcntl_getfl(expected_fd))
        .WillOnce(Return(flags_blocking));
    EXPECT_CALL(mock, fcntl_setfl(expected_fd, flags_blocking))
        .WillOnce(Return(0));
    fd->setBlocking(true);
}

TEST_F(FdMethodTest, SetBlockingOnNonBlocking)
{
    EXPECT_CALL(mock, fcntl_getfl(expected_fd))
        .WillOnce(Return(flags_noblocking));
    EXPECT_CALL(mock, fcntl_setfl(expected_fd, flags_blocking))
        .WillOnce(Return(0));
    fd->setBlocking(true);
}

TEST_F(FdMethodTest, SetNonBlockingOnBlocking)
{
    EXPECT_CALL(mock, fcntl_getfl(expected_fd))
        .WillOnce(Return(flags_blocking));
    EXPECT_CALL(mock, fcntl_setfl(expected_fd, flags_noblocking))
        .WillOnce(Return(0));
    fd->setBlocking(false);
}

TEST_F(FdMethodTest, SetNonBlockingOnNonBlocking)
{
    EXPECT_CALL(mock, fcntl_getfl(expected_fd))
        .WillOnce(Return(flags_noblocking));
    EXPECT_CALL(mock, fcntl_setfl(expected_fd, flags_noblocking))
        .WillOnce(Return(0));
    fd->setBlocking(false);
}

TEST_F(FdMethodTest, GetFlagsFail)
{
    EXPECT_CALL(mock, fcntl_getfl(expected_fd))
        .WillOnce(DoAll(Assign(&errno, EINVAL), Return(-1)));
    EXPECT_THROW(fd->setBlocking(true), std::system_error);
}

TEST_F(FdMethodTest, SetFlagsFail)
{
    EXPECT_CALL(mock, fcntl_getfl(expected_fd)).WillOnce(Return(0));
    EXPECT_CALL(mock, fcntl_setfl(expected_fd, 0))
        .WillOnce(DoAll(Assign(&errno, EINVAL), Return(-1)));
    EXPECT_THROW(fd->setBlocking(true), std::system_error);
}

} // namespace
} // namespace internal
} // namespace gpioplus
