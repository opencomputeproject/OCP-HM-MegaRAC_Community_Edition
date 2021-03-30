#include "generated.hpp"

#include <gtest/gtest.h>

using namespace phosphor::dbus::monitoring;

TEST(JournalTest, Test)
{
    // No assertions here, but the least we can do
    // make sure the program runs without crashing...
    for (auto& c : ConfigPropertyCallbacks::get())
    {
        (*c)(Context::START);
    }
}
