#include "config.h"

#include "phosphor-rsyslog-config/server-conf.hpp"

#include <experimental/filesystem>
#include <sdbusplus/bus.hpp>

#include "gmock/gmock.h"
#include <gtest/gtest.h>

namespace phosphor
{
namespace logging
{
namespace test
{

namespace fs = std::experimental::filesystem;

char tmplt[] = "/tmp/logging_test.XXXXXX";
auto bus = sdbusplus::bus::new_default();
fs::path dir(fs::path(mkdtemp(tmplt)));

class MockServer : public phosphor::rsyslog_config::Server
{
  public:
    MockServer(sdbusplus::bus::bus& bus, const std::string& path,
               const char* filePath) :
        phosphor::rsyslog_config::Server(bus, path, filePath)
    {
    }

    MOCK_METHOD0(restart, void());
};

class TestRemoteLogging : public testing::Test
{
  public:
    TestRemoteLogging()
    {
        configFilePath = std::string(dir.c_str()) + "/server.conf";
        config = new MockServer(bus, BUSPATH_REMOTE_LOGGING_CONFIG,
                                configFilePath.c_str());
    }

    ~TestRemoteLogging()
    {
        delete config;
    }

    static void TearDownTestCase()
    {
        fs::remove_all(dir);
    }

    MockServer* config;
    std::string configFilePath;
};

} // namespace test
} // namespace logging
} // namespace phosphor
