#include <gtest/gtest.h>
#include <stdplus/util/cexec.hpp>
#include <string_view>
#include <system_error>

namespace stdplus
{
namespace util
{
namespace
{

int sample1()
{
    return 1;
}

int sample2(int val)
{
    return val;
}

ssize_t sample3(int val, ssize_t* val2)
{
    return *val2 + val;
}

const char* ptr(const char* p)
{
    return p;
}

struct sample
{
    int count = 3;

    int one()
    {
        return count++;
    }

    int two(int val) const
    {
        return val;
    }

    int* ptr()
    {
        return &count;
    }

    int* ptr2()
    {
        return nullptr;
    }

    static int s(int val)
    {
        return val;
    }
};

int makeTrivialError(int error, const char* msg)
{
    (void)msg;
    return error;
}

TEST(Cexec, CallCheckErrnoInt)
{
    EXPECT_EQ(1, callCheckErrno("sample1", sample1));
    EXPECT_EQ(2, callCheckErrno(std::string("sample2"), &sample2, 2));
    EXPECT_EQ(4, callCheckErrno("sample::s", sample::s, 4));
    ssize_t v = 10;
    EXPECT_EQ(12, callCheckErrno("sample3", sample3, 2, &v));

    constexpr auto error = "sample2 error";
    try
    {
        errno = EBADF;
        callCheckErrno(error, sample2, -1);
        EXPECT_TRUE(false);
    }
    catch (const std::system_error& e)
    {
        EXPECT_EQ(std::string_view(error),
                  std::string_view(e.what(), strlen(error)));
        EXPECT_EQ(EBADF, e.code().value());
    }
}

TEST(Cexec, CallCheckErrnoIntMem)
{
    sample s;
    const sample* sp = &s;
    EXPECT_EQ(3, callCheckErrno("sample::one", &sample::one, s));
    EXPECT_EQ(4, callCheckErrno("sample::one", &sample::one, &s));
    EXPECT_EQ(5, callCheckErrno("sample::two", &sample::two, sp, 5));

    constexpr auto error = "sample error";
    try
    {
        errno = EBADF;
        callCheckErrno(error, &sample::two, sp, -1);
        EXPECT_TRUE(false);
    }
    catch (const std::system_error& e)
    {
        EXPECT_EQ(std::string_view(error),
                  std::string_view(e.what(), strlen(error)));
        EXPECT_EQ(EBADF, e.code().value());
    }
}

TEST(Cexec, CallCheckErrnoPtr)
{
    constexpr auto sample = "sample";
    EXPECT_EQ(sample, callCheckErrno("sample1", ptr, sample));

    constexpr auto error = "sample error";
    try
    {
        errno = EBADF;
        callCheckErrno(error, &ptr, nullptr);
        EXPECT_TRUE(false);
    }
    catch (const std::system_error& e)
    {
        EXPECT_EQ(std::string_view(error),
                  std::string_view(e.what(), strlen(error)));
        EXPECT_EQ(EBADF, e.code().value());
    }
}

TEST(Cexec, CallCheckErrnoPtrMem)
{
    sample s;
    EXPECT_EQ(&s.count, callCheckErrno("sample1", &sample::ptr, &s));

    constexpr auto error = "sample error";
    try
    {
        errno = EBADF;
        callCheckErrno(error, &sample::ptr2, s);
        EXPECT_TRUE(false);
    }
    catch (const std::system_error& e)
    {
        EXPECT_EQ(std::string_view(error),
                  std::string_view(e.what(), strlen(error)));
        EXPECT_EQ(EBADF, e.code().value());
    }
}

TEST(Cexec, CallCheckErrnoErrorFunc)
{
    errno = EBADF;
    try
    {
        callCheckErrno<makeTrivialError>("sample2", sample2, -1);
        EXPECT_TRUE(false);
    }
    catch (int error)
    {
        EXPECT_EQ(errno, error);
    }
}

TEST(Cexec, CallCheckRetInt)
{
    EXPECT_EQ(1, callCheckRet("sample1", sample1));
    EXPECT_EQ(2, callCheckRet(std::string("sample2"), &sample2, 2));
    EXPECT_EQ(4, callCheckRet("sample::s", sample::s, 4));
    ssize_t v = 10;
    EXPECT_EQ(12, callCheckRet("sample3", sample3, 2, &v));

    constexpr auto error = "sample2 error";
    try
    {
        errno = EBADF;
        callCheckRet(error, sample2, -EINTR);
        EXPECT_TRUE(false);
    }
    catch (const std::system_error& e)
    {
        EXPECT_EQ(std::string_view(error),
                  std::string_view(e.what(), strlen(error)));
        EXPECT_EQ(EINTR, e.code().value());
    }
}

TEST(Cexec, CallCheckRetIntMem)
{
    sample s;
    const sample* sp = &s;
    EXPECT_EQ(3, callCheckRet("sample::one", &sample::one, s));
    EXPECT_EQ(4, callCheckRet("sample::one", &sample::one, &s));
    EXPECT_EQ(5, callCheckRet("sample::two", &sample::two, sp, 5));

    constexpr auto error = "sample error";
    try
    {
        errno = EBADF;
        callCheckRet(error, &sample::two, s, -EINTR);
        EXPECT_TRUE(false);
    }
    catch (const std::system_error& e)
    {
        EXPECT_EQ(std::string_view(error),
                  std::string_view(e.what(), strlen(error)));
        EXPECT_EQ(EINTR, e.code().value());
    }
}

TEST(Cexec, CallCheckRetErrorFunc)
{
    try
    {
        callCheckRet<makeTrivialError>("sample2", sample2, -EBADF);
        EXPECT_TRUE(false);
    }
    catch (int error)
    {
        EXPECT_EQ(EBADF, error);
    }
}

} // namespace
} // namespace util
} // namespace stdplus
