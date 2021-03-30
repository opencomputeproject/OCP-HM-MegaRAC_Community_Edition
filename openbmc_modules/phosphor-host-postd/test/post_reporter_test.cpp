#include "lpcsnoop/snoop.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::IsNull;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrEq;

namespace
{

// Fixture for testing class PostReporter
class PostReporterTest : public ::testing::Test
{
  protected:
    PostReporterTest() : bus_mock(), bus(sdbusplus::get_mocked_new(&bus_mock))
    {
    }

    ~PostReporterTest()
    {
    }

    NiceMock<sdbusplus::SdBusMock> bus_mock;
    sdbusplus::bus::bus bus;
};

TEST_F(PostReporterTest, EmitsObjectsOnExpectedDbusPath)
{

    EXPECT_CALL(bus_mock,
                sd_bus_emit_object_added(IsNull(), StrEq(SNOOP_OBJECTPATH)))
        .WillOnce(Return(0));

    PostReporter testReporter(bus, SNOOP_OBJECTPATH, true);
    testReporter.emit_object_added();
}

TEST_F(PostReporterTest, AddsObjectWithExpectedName)
{
    EXPECT_CALL(bus_mock,
                sd_bus_add_object_vtable(IsNull(), _, StrEq(SNOOP_OBJECTPATH),
                                         StrEq(SNOOP_BUSNAME), _, _))
        .WillOnce(Return(0));

    PostReporter testReporter(bus, SNOOP_OBJECTPATH, true);
}

TEST_F(PostReporterTest, ValueReadsDefaultToZero)
{
    PostReporter testReporter(bus, SNOOP_OBJECTPATH, true);
    EXPECT_EQ(0, testReporter.value());
}

TEST_F(PostReporterTest, SetValueToPositiveValueWorks)
{
    PostReporter testReporter(bus, SNOOP_OBJECTPATH, true);
    testReporter.value(65537);
    EXPECT_EQ(65537, testReporter.value());
}

TEST_F(PostReporterTest, SetValueMultipleTimesWorks)
{
    PostReporter testReporter(bus, SNOOP_OBJECTPATH, true);
    testReporter.value(123);
    EXPECT_EQ(123, testReporter.value());
    testReporter.value(456);
    EXPECT_EQ(456, testReporter.value());
    testReporter.value(0);
    EXPECT_EQ(0, testReporter.value());
    testReporter.value(456);
    EXPECT_EQ(456, testReporter.value());
    testReporter.value(456);
    EXPECT_EQ(456, testReporter.value());
}

} // namespace
