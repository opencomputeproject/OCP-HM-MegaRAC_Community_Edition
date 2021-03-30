#include "src/associations.hpp"

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

/* @brief Will contain path and name of test application */
const char* appname = program_invocation_name;

#include <gtest/gtest.h>
/** @class AsioServerClassTest
 *
 *  @brief Provide wrapper for creating asio::object_server for test suite
 */
class AsioServerClassTest : public testing::Test
{
  protected:
    // Make this global to the whole test suite since we want to share
    // the asio::object_server accross the test cases
    // NOTE - latest googltest changed to SetUpTestSuite()
    static void SetUpTestCase()
    {
        boost::asio::io_context io;
        auto conn = std::make_shared<sdbusplus::asio::connection>(io);

        // Need a distinct name for the bus since multiple test applications
        // will be running at same time
        std::string dbusName = {"xyz.openbmc_project.ObjMgr.Test."};
        std::string fullAppPath = {appname};
        std::size_t fileNameLoc = fullAppPath.find_last_of("/\\");
        dbusName += fullAppPath.substr(fileNameLoc + 1);
        conn->request_name(dbusName.c_str());
        server = new sdbusplus::asio::object_server(conn);
    }

    // NOTE - latest googltest changed to TearDownTestSuite()
    static void TearDownTestCase()
    {
        delete server;
        server = nullptr;
    }

    static sdbusplus::asio::object_server* server;
};
