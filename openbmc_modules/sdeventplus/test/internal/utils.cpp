#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <sdeventplus/internal/utils.hpp>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace sdeventplus
{
namespace internal
{
namespace
{

TEST(UtilsTest, PerformCallbackSuccess)
{
    EXPECT_EQ(0, performCallback(nullptr, []() {}));
}

TEST(UtilsTest, PerformCallbackAcceptsReference)
{
    auto f =
        std::bind([](const std::unique_ptr<int>&) {}, std::make_unique<int>(1));
    EXPECT_EQ(0, performCallback(nullptr, f));
}

TEST(UtilsTest, PerformCallbackAcceptsMove)
{
    auto f =
        std::bind([](const std::unique_ptr<int>&) {}, std::make_unique<int>(1));
    EXPECT_EQ(0, performCallback(nullptr, std::move(f)));
}

TEST(UtilsTest, SetPrepareSystemError)
{
    EXPECT_EQ(-EBUSY, performCallback("system_error", []() {
        throw std::system_error(EBUSY, std::generic_category());
    }));
}

TEST(UtilsTest, SetPrepareException)
{
    EXPECT_EQ(-ENOSYS, performCallback("runtime_error", []() {
        throw std::runtime_error("Exception");
    }));
}

TEST(UtilsTest, SetPrepareUnknownException)
{
    EXPECT_EQ(-ENOSYS,
              performCallback("unknown", []() { throw static_cast<int>(1); }));
}

} // namespace
} // namespace internal
} // namespace sdeventplus
