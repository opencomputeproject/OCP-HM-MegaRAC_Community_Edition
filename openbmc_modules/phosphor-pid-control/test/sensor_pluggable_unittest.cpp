#include "sensors/pluggable.hpp"
#include "test/readinterface_mock.hpp"
#include "test/writeinterface_mock.hpp"

#include <chrono>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Invoke;

TEST(PluggableSensorTest, BoringConstructorTest)
{
    // Build a boring Pluggable Sensor.

    int64_t min = 0;
    int64_t max = 255;

    std::unique_ptr<ReadInterface> ri = std::make_unique<ReadInterfaceMock>();
    std::unique_ptr<WriteInterface> wi =
        std::make_unique<WriteInterfaceMock>(min, max);

    std::string name = "name";
    int64_t timeout = 1;

    PluggableSensor p(name, timeout, std::move(ri), std::move(wi));
    // Successfully created it.
}

TEST(PluggableSensorTest, TryReadingTest)
{
    // Verify calling read, calls the ReadInterface.

    int64_t min = 0;
    int64_t max = 255;

    std::unique_ptr<ReadInterface> ri = std::make_unique<ReadInterfaceMock>();
    std::unique_ptr<WriteInterface> wi =
        std::make_unique<WriteInterfaceMock>(min, max);

    std::string name = "name";
    int64_t timeout = 1;

    ReadInterfaceMock* rip = reinterpret_cast<ReadInterfaceMock*>(ri.get());

    PluggableSensor p(name, timeout, std::move(ri), std::move(wi));

    ReadReturn r;
    r.value = 0.1;
    r.updated = std::chrono::high_resolution_clock::now();

    EXPECT_CALL(*rip, read()).WillOnce(Invoke([&](void) { return r; }));

    // TODO(venture): Implement comparison operator for ReadReturn.
    ReadReturn v = p.read();
    EXPECT_EQ(r.value, v.value);
    EXPECT_EQ(r.updated, v.updated);
}

TEST(PluggableSensorTest, TryWritingTest)
{
    // Verify calling write, calls the WriteInterface.

    int64_t min = 0;
    int64_t max = 255;

    std::unique_ptr<ReadInterface> ri = std::make_unique<ReadInterfaceMock>();
    std::unique_ptr<WriteInterface> wi =
        std::make_unique<WriteInterfaceMock>(min, max);

    std::string name = "name";
    int64_t timeout = 1;

    WriteInterfaceMock* wip = reinterpret_cast<WriteInterfaceMock*>(wi.get());

    PluggableSensor p(name, timeout, std::move(ri), std::move(wi));

    double value = 0.303;

    EXPECT_CALL(*wip, write(value));
    p.write(value);
}
