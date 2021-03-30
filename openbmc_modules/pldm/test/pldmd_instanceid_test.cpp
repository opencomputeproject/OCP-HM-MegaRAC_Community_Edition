#include "pldmd/instance_id.hpp"

#include <stdexcept>

#include <gtest/gtest.h>

using namespace pldm;

TEST(InstanceId, testNext)
{
    InstanceId id;
    ASSERT_EQ(id.next(), 0);
    ASSERT_EQ(id.next(), 1);
}

TEST(InstanceId, testAllUsed)
{
    InstanceId id;
    for (size_t i = 0; i < maxInstanceIds; ++i)
    {
        ASSERT_EQ(id.next(), i);
    }
    EXPECT_THROW(id.next(), std::runtime_error);
}

TEST(InstanceId, testMarkfree)
{
    InstanceId id;
    for (size_t i = 0; i < maxInstanceIds; ++i)
    {
        ASSERT_EQ(id.next(), i);
    }
    id.markFree(5);
    ASSERT_EQ(id.next(), 5);
    id.markFree(0);
    ASSERT_EQ(id.next(), 0);
    EXPECT_THROW(id.next(), std::runtime_error);
    EXPECT_THROW(id.markFree(32), std::out_of_range);
}
