#include "../serialize.hpp"

#include <gtest/gtest.h>

using namespace phosphor::inventory::manager;
using namespace std::string_literals;

TEST(SerializeTest, TestStoragePathNoSlashes)
{
    auto path = "foo/bar/baz"s;
    auto iface = "xyz.foo"s;
    auto p1 = detail::getStoragePath(path, iface);
    auto p2 = fs::path(PIM_PERSIST_PATH "/foo/bar/baz/xyz.foo");
    EXPECT_EQ(p1, p2);
}

TEST(SerializeTest, TestStoragePathSlashes)
{
    auto path = "/foo/bar/baz"s;
    auto iface = "/xyz.foo"s;
    auto p1 = detail::getStoragePath(path, iface);
    auto p2 = fs::path(PIM_PERSIST_PATH "/foo/bar/baz/xyz.foo");
    EXPECT_EQ(p1, p2);
}
