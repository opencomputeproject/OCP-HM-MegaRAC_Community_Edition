#include "config.h"

#include "log_manager.hpp"

#include <stdlib.h>

#include <experimental/filesystem>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <gtest/gtest.h>

namespace phosphor
{
namespace logging
{
namespace test
{

namespace fs = std::experimental::filesystem;

char tmplt[] = "/tmp/logging_test.XXXXXX";
sdbusplus::SdBusMock sdbusMock;
sdbusplus::bus::bus bus = sdbusplus::get_mocked_new(&sdbusMock);
phosphor::logging::internal::Manager manager(bus, OBJ_INTERNAL);

class TestSerialization : public testing::Test
{
  public:
    TestSerialization() : dir(fs::path(mkdtemp(tmplt)))
    {
    }

    ~TestSerialization()
    {
        fs::remove_all(dir);
    }

    fs::path dir;
};

} // namespace test
} // namespace logging
} // namespace phosphor
