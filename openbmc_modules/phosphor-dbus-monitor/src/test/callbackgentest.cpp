#include "data_types.hpp"

#include <array>
#include <string>

#include <gtest/gtest.h>

using namespace phosphor::dbus::monitoring;

using Index = std::map<std::tuple<size_t, size_t, size_t>, size_t>;

#include "callbackgentest.hpp"

const std::array<std::tuple<std::string, size_t>, 4> expectedCallbacks = {{
    std::tuple<std::string, size_t>{"int32_t", 0},
    std::tuple<std::string, size_t>{"int32_t", 0},
    std::tuple<std::string, size_t>{"std::string", 1},
    std::tuple<std::string, size_t>{"std::string", 2},
}};

TEST(CallbackGenTest, CallbacksSameSize)
{
    ASSERT_EQ(sizeof(expectedCallbacks), sizeof(callbacks));
}

TEST(CallbackGenTest, CallbacksSameContent)
{
    size_t i;
    for (i = 0; i < expectedCallbacks.size(); ++i)
    {
        ASSERT_EQ(callbacks[i], expectedCallbacks[i]);
    }
}
