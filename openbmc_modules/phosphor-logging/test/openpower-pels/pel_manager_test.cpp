/**
 * Copyright © 2019 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "extensions/openpower-pels/manager.hpp"
#include "log_manager.hpp"
#include "mocks.hpp"
#include "pel_utils.hpp"

#include <fstream>
#include <regex>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <gtest/gtest.h>

using namespace openpower::pels;
namespace fs = std::filesystem;

using ::testing::NiceMock;

class TestLogger
{
  public:
    void log(const std::string& name, phosphor::logging::Entry::Level level,
             const EventLogger::ADMap& additionalData)
    {
        errName = name;
        errLevel = level;
        ad = additionalData;
    }

    std::string errName;
    phosphor::logging::Entry::Level errLevel;
    EventLogger::ADMap ad;
};

class ManagerTest : public CleanPELFiles
{
  public:
    ManagerTest() :
        bus(sdbusplus::get_mocked_new(&sdbusInterface)),
        logManager(bus, "logging_path")
    {
        sd_event_default(&sdEvent);
    }

    ~ManagerTest()
    {
        sd_event_unref(sdEvent);
    }

    NiceMock<sdbusplus::SdBusMock> sdbusInterface;
    sdbusplus::bus::bus bus;
    phosphor::logging::internal::Manager logManager;
    sd_event* sdEvent;
    TestLogger logger;
};

fs::path makeTempDir()
{
    char path[] = "/tmp/tempnameXXXXXX";
    std::filesystem::path dir = mkdtemp(path);
    return dir;
}

std::optional<fs::path> findAnyPELInRepo()
{
    // PELs are named <timestamp>_<ID>
    std::regex expr{"\\d+_\\d+"};

    for (auto& f : fs::directory_iterator(getPELRepoPath() / "logs"))
    {
        if (std::regex_search(f.path().string(), expr))
        {
            return f.path();
        }
    }
    return std::nullopt;
}

size_t countPELsInRepo()
{
    size_t count = 0;
    std::regex expr{"\\d+_\\d+"};

    for (auto& f : fs::directory_iterator(getPELRepoPath() / "logs"))
    {
        if (std::regex_search(f.path().string(), expr))
        {
            count++;
        }
    }
    return count;
}

void deletePELFile(uint32_t id)
{
    char search[20];

    sprintf(search, "\\d+_%.8X", id);
    std::regex expr{search};

    for (auto& f : fs::directory_iterator(getPELRepoPath() / "logs"))
    {
        if (std::regex_search(f.path().string(), expr))
        {
            fs::remove(f.path());
            break;
        }
    }
}

// Test that using the RAWPEL=<file> with the Manager::create() call gets
// a PEL saved in the repository.
TEST_F(ManagerTest, TestCreateWithPEL)
{
    std::unique_ptr<DataInterfaceBase> dataIface =
        std::make_unique<MockDataInterface>();

    openpower::pels::Manager manager{
        logManager, std::move(dataIface),
        std::bind(std::mem_fn(&TestLogger::log), &logger, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)};

    // Create a PEL, write it to a file, and pass that filename into
    // the create function.
    auto data = pelDataFactory(TestPELType::pelSimple);

    fs::path pelFilename = makeTempDir() / "rawpel";
    std::ofstream pelFile{pelFilename};
    pelFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    pelFile.close();

    std::string adItem = "RAWPEL=" + pelFilename.string();
    std::vector<std::string> additionalData{adItem};
    std::vector<std::string> associations;

    manager.create("error message", 42, 0,
                   phosphor::logging::Entry::Level::Error, additionalData,
                   associations);

    // Find the file in the PEL repository directory
    auto pelPathInRepo = findAnyPELInRepo();

    EXPECT_TRUE(pelPathInRepo);

    // Now remove it based on its OpenBMC event log ID
    manager.erase(42);

    pelPathInRepo = findAnyPELInRepo();

    EXPECT_FALSE(pelPathInRepo);

    fs::remove_all(pelFilename.parent_path());
}

TEST_F(ManagerTest, TestCreateWithInvalidPEL)
{
    std::unique_ptr<DataInterfaceBase> dataIface =
        std::make_unique<MockDataInterface>();

    openpower::pels::Manager manager{
        logManager, std::move(dataIface),
        std::bind(std::mem_fn(&TestLogger::log), &logger, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)};

    // Create a PEL, write it to a file, and pass that filename into
    // the create function.
    auto data = pelDataFactory(TestPELType::pelSimple);

    // Truncate it to make it invalid.
    data.resize(200);

    fs::path pelFilename = makeTempDir() / "rawpel";
    std::ofstream pelFile{pelFilename};
    pelFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    pelFile.close();

    std::string adItem = "RAWPEL=" + pelFilename.string();
    std::vector<std::string> additionalData{adItem};
    std::vector<std::string> associations;

    manager.create("error message", 42, 0,
                   phosphor::logging::Entry::Level::Error, additionalData,
                   associations);

    // Run the event loop to log the bad PEL event
    sdeventplus::Event e{sdEvent};
    e.run(std::chrono::milliseconds(1));

    PEL invalidPEL{data};
    EXPECT_EQ(logger.errName, "org.open_power.Logging.Error.BadHostPEL");
    EXPECT_EQ(logger.errLevel, phosphor::logging::Entry::Level::Error);
    EXPECT_EQ(std::stoi(logger.ad["PLID"], nullptr, 16), invalidPEL.plid());
    EXPECT_EQ(logger.ad["OBMC_LOG_ID"], "42");
    EXPECT_EQ(logger.ad["SRC"], (*invalidPEL.primarySRC())->asciiString());
    EXPECT_EQ(logger.ad["PEL_SIZE"], std::to_string(data.size()));

    // Check that the bad PEL data was saved to a file.
    auto badPELData = readPELFile(getPELRepoPath() / "badPEL");
    EXPECT_EQ(*badPELData, data);

    fs::remove_all(pelFilename.parent_path());
}

// Test that the message registry can be used to build a PEL.
TEST_F(ManagerTest, TestCreateWithMessageRegistry)
{
    const auto registry = R"(
{
    "PELs":
    [
        {
            "Name": "xyz.openbmc_project.Error.Test",
            "Subsystem": "power_supply",
            "ActionFlags": ["service_action", "report"],
            "SRC":
            {
                "ReasonCode": "0x2030"
            },
            "Documentation":
            {
                "Description": "A PGOOD Fault",
                "Message": "PS had a PGOOD Fault"
            }
        }
    ]
}
)";

    auto path = getPELReadOnlyDataPath();
    fs::create_directories(path);
    path /= "message_registry.json";

    std::ofstream registryFile{path};
    registryFile << registry;
    registryFile.close();

    std::unique_ptr<DataInterfaceBase> dataIface =
        std::make_unique<MockDataInterface>();

    openpower::pels::Manager manager{
        logManager, std::move(dataIface),
        std::bind(std::mem_fn(&TestLogger::log), &logger, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)};

    std::vector<std::string> additionalData;
    std::vector<std::string> associations;

    // Create the event log to create the PEL from.
    manager.create("xyz.openbmc_project.Error.Test", 33, 0,
                   phosphor::logging::Entry::Level::Error, additionalData,
                   associations);

    // Ensure a PEL was created in the repository
    auto pelFile = findAnyPELInRepo();
    ASSERT_TRUE(pelFile);

    auto data = readPELFile(*pelFile);
    PEL pel(*data);

    // Spot check it.  Other testcases cover the details.
    EXPECT_TRUE(pel.valid());
    EXPECT_EQ(pel.obmcLogID(), 33);
    EXPECT_EQ(pel.primarySRC().value()->asciiString(),
              "BD612030                        ");

    // Remove it
    manager.erase(33);
    pelFile = findAnyPELInRepo();
    EXPECT_FALSE(pelFile);

    // Create an event log that can't be found in the registry.
    manager.create("xyz.openbmc_project.Error.Foo", 33, 0,
                   phosphor::logging::Entry::Level::Error, additionalData,
                   associations);

    // Currently, no PEL should be created.  Eventually, a 'missing registry
    // entry' PEL will be there.
    pelFile = findAnyPELInRepo();
    EXPECT_FALSE(pelFile);
}

TEST_F(ManagerTest, TestDBusMethods)
{
    std::unique_ptr<DataInterfaceBase> dataIface =
        std::make_unique<MockDataInterface>();

    Manager manager{logManager, std::move(dataIface),
                    std::bind(std::mem_fn(&TestLogger::log), &logger,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3)};

    // Create a PEL, write it to a file, and pass that filename into
    // the create function so there's one in the repo.
    auto data = pelDataFactory(TestPELType::pelSimple);

    fs::path pelFilename = makeTempDir() / "rawpel";
    std::ofstream pelFile{pelFilename};
    pelFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    pelFile.close();

    std::string adItem = "RAWPEL=" + pelFilename.string();
    std::vector<std::string> additionalData{adItem};
    std::vector<std::string> associations;

    manager.create("error message", 42, 0,
                   phosphor::logging::Entry::Level::Error, additionalData,
                   associations);

    // getPELFromOBMCID
    auto newData = manager.getPELFromOBMCID(42);
    EXPECT_EQ(newData.size(), data.size());

    // Read the PEL to get the ID for later
    PEL pel{newData};
    auto id = pel.id();

    EXPECT_THROW(
        manager.getPELFromOBMCID(id + 1),
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument);

    // getPEL
    auto unixfd = manager.getPEL(id);

    // Get the size
    struct stat s;
    int r = fstat(unixfd, &s);
    ASSERT_EQ(r, 0);
    auto size = s.st_size;

    // Open the FD and check the contents
    FILE* fp = fdopen(unixfd, "r");
    ASSERT_NE(fp, nullptr);

    std::vector<uint8_t> fdData;
    fdData.resize(size);
    r = fread(fdData.data(), 1, size, fp);
    EXPECT_EQ(r, size);

    EXPECT_EQ(newData, fdData);

    fclose(fp);

    // Run the event loop to close the FD
    sdeventplus::Event e{sdEvent};
    e.run(std::chrono::milliseconds(1));

    EXPECT_THROW(
        manager.getPEL(id + 1),
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument);

    // hostAck
    manager.hostAck(id);

    EXPECT_THROW(
        manager.hostAck(id + 1),
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument);

    // hostReject
    manager.hostReject(id, Manager::RejectionReason::BadPEL);

    // Run the event loop to log the bad PEL event
    e.run(std::chrono::milliseconds(1));

    EXPECT_EQ(logger.errName, "org.open_power.Logging.Error.SentBadPELToHost");
    EXPECT_EQ(id, std::stoi(logger.ad["BAD_ID"], nullptr, 16));

    manager.hostReject(id, Manager::RejectionReason::HostFull);

    EXPECT_THROW(
        manager.hostReject(id + 1, Manager::RejectionReason::BadPEL),
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument);

    fs::remove_all(pelFilename.parent_path());
}

// An ESEL from the wild
const std::string esel{
    "00 00 df 00 00 00 00 20 00 04 12 01 6f aa 00 00 "
    "50 48 00 30 01 00 33 00 00 00 00 07 5c 69 cc 0d 00 00 00 07 5c d5 50 db "
    "42 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00 90 00 00 4e 90 00 00 4e "
    "55 48 00 18 01 00 09 00 8a 03 40 00 00 00 00 00 ff ff 00 00 00 00 00 00 "
    "50 53 00 50 01 01 00 00 02 00 00 09 33 2d 00 48 00 00 00 e0 00 00 10 00 "
    "00 00 00 00 00 20 00 00 00 0c 00 02 00 00 00 fa 00 00 0c e4 00 00 00 12 "
    "42 43 38 41 33 33 32 44 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 "
    "20 20 20 20 20 20 20 20 55 44 00 1c 01 06 01 00 02 54 41 4b 00 00 00 06 "
    "00 00 00 55 00 01 f9 20 00 00 00 00 55 44 00 24 01 06 01 00 01 54 41 4b "
    "00 00 00 05 00 00 00 00 00 00 00 00 00 00 00 00 23 01 00 02 00 05 00 00 "
    "55 44 00 0c 01 0b 01 00 0f 01 00 00 55 44 00 10 01 04 01 00 0f 9f de 6a "
    "00 01 00 00 55 44 00 7c 00 0c 01 00 00 13 0c 02 00 fa 0c e4 16 00 01 2c "
    "0c 1c 16 00 00 fa 0a f0 14 00 00 fa 0b b8 14 00 00 be 09 60 12 00 01 2c "
    "0d 7a 12 00 00 fa 0c 4e 10 00 00 fa 0c e4 10 00 00 be 0a 8c 16 00 01 2c "
    "0c 1c 16 00 01 09 09 f6 16 00 00 fa 09 f6 14 00 00 fa 0b b8 14 00 00 fa "
    "0a f0 14 00 00 be 08 ca 12 00 01 2c 0c e4 12 00 00 fa 0b 54 10 00 00 fa "
    "0c 2d 10 00 00 be 08 ca 55 44 00 58 01 03 01 00 00 00 00 00 00 05 31 64 "
    "00 00 00 00 00 05 0d d4 00 00 00 00 40 5f 06 e0 00 00 00 00 40 5d d2 00 "
    "00 00 00 00 40 57 d3 d0 00 00 00 00 40 58 f6 a0 00 00 00 00 40 54 c9 34 "
    "00 00 00 00 40 55 9a 10 00 00 00 00 40 4c 0a 80 00 00 00 00 00 00 27 14 "
    "55 44 01 84 01 01 01 00 48 6f 73 74 62 6f 6f 74 20 42 75 69 6c 64 20 49 "
    "44 3a 20 68 6f 73 74 62 6f 6f 74 2d 66 65 63 37 34 64 66 2d 70 30 61 38 "
    "37 64 63 34 2f 68 62 69 63 6f 72 65 2e 62 69 6e 00 49 42 4d 2d 77 69 74 "
    "68 65 72 73 70 6f 6f 6e 2d 4f 50 39 2d 76 32 2e 34 2d 39 2e 32 33 34 0a "
    "09 6f 70 2d 62 75 69 6c 64 2d 38 32 66 34 63 66 30 0a 09 62 75 69 6c 64 "
    "72 6f 6f 74 2d 32 30 31 39 2e 30 35 2e 32 2d 31 30 2d 67 38 39 35 39 31 "
    "31 34 0a 09 73 6b 69 62 6f 6f 74 2d 76 36 2e 35 2d 31 38 2d 67 34 37 30 "
    "66 66 62 35 66 32 39 64 37 0a 09 68 6f 73 74 62 6f 6f 74 2d 66 65 63 37 "
    "34 64 66 2d 70 30 61 38 37 64 63 34 0a 09 6f 63 63 2d 65 34 35 39 37 61 "
    "62 0a 09 6c 69 6e 75 78 2d 35 2e 32 2e 31 37 2d 6f 70 65 6e 70 6f 77 65 "
    "72 31 2d 70 64 64 63 63 30 33 33 0a 09 70 65 74 69 74 62 6f 6f 74 2d 76 "
    "31 2e 31 30 2e 34 0a 09 6d 61 63 68 69 6e 65 2d 78 6d 6c 2d 63 36 32 32 "
    "63 62 35 2d 70 37 65 63 61 62 33 64 0a 09 68 6f 73 74 62 6f 6f 74 2d 62 "
    "69 6e 61 72 69 65 73 2d 36 36 65 39 61 36 30 0a 09 63 61 70 70 2d 75 63 "
    "6f 64 65 2d 70 39 2d 64 64 32 2d 76 34 0a 09 73 62 65 2d 36 30 33 33 30 "
    "65 30 0a 09 68 63 6f 64 65 2d 68 77 30 39 32 31 31 39 61 2e 6f 70 6d 73 "
    "74 0a 00 00 55 44 00 70 01 04 01 00 0f 9f de 6a 00 05 00 00 07 5f 1d f4 "
    "30 32 43 59 34 37 30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
    "0b ac 54 02 59 41 31 39 33 34 36 39 37 30 35 38 00 00 00 00 00 00 05 22 "
    "a1 58 01 8a 00 58 40 20 17 18 4d 2c 00 00 00 fc 01 a1 00 00 55 44 00 14 "
    "01 08 01 00 00 00 00 01 00 00 00 5a 00 00 00 05 55 44 03 fc 01 15 31 00 "
    "01 28 00 42 46 41 50 49 00 00 00 00 00 00 00 00 00 00 00 00 00 00 03 f4 "
    "00 00 00 00 00 00 03 f4 00 00 00 0b 00 00 00 00 00 00 00 3d 2c 9b c2 84 "
    "00 00 01 e4 00 48 43 4f fb ed 70 b1 00 00 02 01 00 00 00 00 00 00 00 09 "
    "00 00 00 00 00 11 bd 20 00 00 00 00 00 01 f8 80 00 00 00 00 00 00 00 01 "
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 16 00 00 00 00 00 00 01 2c "
    "00 00 00 00 00 00 07 d0 00 00 00 00 00 00 0c 1c 00 00 00 64 00 00 00 3d "
    "2c 9b d1 11 00 00 01 e4 00 48 43 4f fb ed 70 b1 00 00 02 01 00 00 00 00 "
    "00 00 00 0a 00 00 00 00 00 13 b5 a0 00 00 00 00 00 01 f8 80 00 00 00 00 "
    "00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 10 00 00 00 00 "
    "00 00 00 be 00 00 00 00 00 00 07 d0 00 00 00 00 00 00 0a 8c 00 00 00 64 "
    "00 00 00 3d 2c 9b df 98 00 00 01 e4 00 48 43 4f fb ed 70 b1 00 00 02 01 "
    "00 00 00 00 00 00 00 0b 00 00 00 00 00 15 ae 20 00 00 00 00 00 01 f8 80 "
    "00 00 00 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 10 "
    "00 00 00 00 00 00 00 fa 00 00 00 00 00 00 07 d0 00 00 00 00 00 00 0c e4 "
    "00 00 00 64 00 00 00 3d 2c 9b ea b7 00 00 01 e4 00 48 43 4f fb ed 70 b1 "
    "00 00 02 01 00 00 00 00 00 00 00 0c 00 00 00 00 00 17 a6 a0 00 00 00 00 "
    "00 01 f8 80 00 00 00 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 "
    "00 00 00 12 00 00 00 00 00 00 00 fa 00 00 00 00 00 00 07 d0 00 00 00 00 "
    "00 00 0c 4e 00 00 00 64 00 00 00 3d 2c 9b f6 27 00 00 01 e4 00 48 43 4f "
    "fb ed 70 b1 00 00 02 01 00 00 00 00 00 00 00 0d 00 00 00 00 00 19 9f 20 "
    "00 00 00 00 00 01 f8 80 00 00 00 00 00 00 00 01 00 00 00 00 00 00 00 00 "
    "00 00 00 00 00 00 00 12 00 00 00 00 00 00 01 2c 00 00 00 00 00 00 07 d0 "
    "00 00 00 00 00 00 0d 7a 00 00 00 64 00 00 00 3d 2c 9c 05 75 00 00 01 e4 "
    "00 48 43 4f fb ed 70 b1 00 00 02 01 00 00 00 00 00 00 00 0e 00 00 00 00 "
    "00 1b 97 a0 00 00 00 00 00 01 f8 80 00 00 00 00 00 00 00 01 00 00 00 00 "
    "00 00 00 00 00 00 00 00 00 00 00 14 00 00 00 00 00 00 00 be 00 00 00 00 "
    "00 00 07 d0 00 00 00 00 00 00 09 60 00 00 00 64 00 00 00 3d 2c 9c 11 29 "
    "00 00 01 e4 00 48 43 4f fb ed 70 b1 00 00 02 01 00 00 00 00 00 00 00 0f "
    "00 00 00 00 00 1d 90 20 00 00 00 00 00 01 f8 80 00 00 00 00 00 00 00 01 "
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 14 00 00 00 00 00 00 00 fa "
    "00 00 00 00 00 00 07 d0 00 00 00 00 00 00 0b b8 00 00 00 64 00 00 00 3d "
    "2c 9c 1c 45 00 00 01 e4 00 48 43 4f fb ed 70 b1 00 00 02 01 00 00 00 00 "
    "00 00 00 10 00 00 00 00 00 1f 88 a0 00 00 00 00 00 01 f8 80 00 00 00 00 "
    "00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 16 00 00 00 00 "
    "00 00 00 fa 00 00 00 00 00 00 07 d0 00 00 00 00 00 00 0a f0 00 00 00 64 "
    "00 00 00 3d 2c 9c 2b 14 00 00 01 e4 00 48 43 4f fb ed 70 b1 00 00 02 01 "
    "00 00 00 00 00 00 00 11 00 00 00 00 00 21 81 20 00 00 00 00 00 01 f8 80 "
    "00 00 00 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 16 "
    "00 00 00 00 00 00 01 2c 00 00 00 00 00 00 07 d0 00 00 00 00 00 00 0c 1c "
    "00 00 00 64 00 00 00 3d 2d 6d 8f 9e 00 00 01 e4 00 00 43 4f 52 d7 9c 36 "
    "00 00 04 73 00 00 00 1c 00 00 00 3d 2d 6d 99 ac 00 00 01 e4 00 10 43 4f "
    "3f f2 02 3d 00 00 05 58 00 00 00 00 02 00 00 01 00 00 00 00 00 00 00 40 "
    "00 00 00 2c 55 44 00 30 01 15 31 00 01 28 00 42 46 41 50 49 5f 44 42 47 "
    "00 00 00 00 00 00 00 00 00 00 00 28 00 00 00 00 00 00 00 28 00 00 00 00 "
    "00 00 00 00 55 44 01 74 01 15 31 00 01 28 00 42 46 41 50 49 5f 49 00 00 "
    "00 00 00 00 00 00 00 00 00 00 01 6c 00 00 00 00 00 00 01 6c 00 00 00 0b "
    "00 00 00 00 00 00 00 3c 0d 52 18 5e 00 00 01 e4 00 08 43 4f 46 79 94 13 "
    "00 00 0a 5b 00 00 00 00 00 00 2c 00 00 00 00 24 00 00 00 3c 0d 6b 26 6c "
    "00 00 01 e4 00 00 43 4f 4e 9b 18 74 00 00 01 03 00 00 00 1c 00 00 00 3c "
    "12 b9 2d 13 00 00 01 e4 00 00 43 4f ea 31 ed d4 00 00 05 c4 00 00 00 1c "
    "00 00 00 3c 13 02 73 53 00 00 01 e4 00 00 43 4f ea 31 ed d4 00 00 05 c4 "
    "00 00 00 1c 00 00 00 3c 13 04 7c 94 00 00 01 e4 00 00 43 4f ea 31 ed d4 "
    "00 00 05 c4 00 00 00 1c 00 00 00 3c 13 06 ad e1 00 00 01 e4 00 00 43 4f "
    "ea 31 ed d4 00 00 05 c4 00 00 00 1c 00 00 00 3c 13 07 3f 77 00 00 01 e4 "
    "00 00 43 4f 5e 4a 55 32 00 00 10 f2 00 00 00 1c 00 00 00 3c 13 07 4e e4 "
    "00 00 01 e4 00 00 43 4f 5e 4a 55 32 00 00 0d 68 00 00 00 1c 00 00 00 3c "
    "13 36 79 18 00 00 01 e4 00 00 43 4f ea 31 ed d4 00 00 05 c4 00 00 00 1c "
    "00 00 00 3d 2c 9c 36 70 00 00 01 e4 00 00 43 4f 23 45 90 97 00 00 02 47 "
    "00 00 00 1c 00 00 00 3d 2d 6d a3 ed 00 00 01 e4 00 08 43 4f 74 3a 5b 1a "
    "00 00 04 cc 00 00 00 00 02 00 00 01 00 00 00 24 55 44 00 30 01 15 31 00 "
    "01 28 00 42 53 43 41 4e 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 28 "
    "00 00 00 00 00 00 00 28 00 00 00 00 00 00 00 00"};

TEST_F(ManagerTest, TestESELToRawData)
{
    auto data = Manager::eselToRawData(esel);

    EXPECT_EQ(data.size(), 2464);

    PEL pel{data};
    EXPECT_TRUE(pel.valid());
}

TEST_F(ManagerTest, TestCreateWithESEL)
{
    std::unique_ptr<DataInterfaceBase> dataIface =
        std::make_unique<MockDataInterface>();

    openpower::pels::Manager manager{
        logManager, std::move(dataIface),
        std::bind(std::mem_fn(&TestLogger::log), &logger, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)};

    {
        std::string adItem = "ESEL=" + esel;
        std::vector<std::string> additionalData{adItem};
        std::vector<std::string> associations;

        manager.create("error message", 37, 0,
                       phosphor::logging::Entry::Level::Error, additionalData,
                       associations);

        auto data = manager.getPELFromOBMCID(37);
        PEL pel{data};
        EXPECT_TRUE(pel.valid());
    }

    // Now an invalid one
    {
        std::string adItem = "ESEL=" + esel;

        // Crop it
        adItem.resize(adItem.size() - 300);

        std::vector<std::string> additionalData{adItem};
        std::vector<std::string> associations;

        manager.create("error message", 38, 0,
                       phosphor::logging::Entry::Level::Error, additionalData,
                       associations);

        EXPECT_THROW(
            manager.getPELFromOBMCID(38),
            sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument);

        // Run the event loop to log the bad PEL event
        sdeventplus::Event e{sdEvent};
        e.run(std::chrono::milliseconds(1));

        EXPECT_EQ(logger.errName, "org.open_power.Logging.Error.BadHostPEL");
        EXPECT_EQ(logger.errLevel, phosphor::logging::Entry::Level::Error);
    }
}

// Test that PELs will be pruned when necessary
TEST_F(ManagerTest, TestPruning)
{
    sdeventplus::Event e{sdEvent};

    std::unique_ptr<DataInterfaceBase> dataIface =
        std::make_unique<MockDataInterface>();

    openpower::pels::Manager manager{
        logManager, std::move(dataIface),
        std::bind(std::mem_fn(&TestLogger::log), &logger, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)};

    // Create 25 1000B (4096B on disk each, which is what is used for pruning)
    // BMC non-informational PELs in the 100KB repository.  After the 24th one,
    // the repo will be 96% full and a prune should be triggered to remove all
    // but 7 to get under 30% full.  Then when the 25th is added there will be
    // 8 left.

    auto dir = makeTempDir();
    for (int i = 1; i <= 25; i++)
    {
        auto data = pelFactory(42, 'O', 0x40, 0x8800, 1000);

        fs::path pelFilename = dir / "rawpel";
        std::ofstream pelFile{pelFilename};
        pelFile.write(reinterpret_cast<const char*>(data.data()), data.size());
        pelFile.close();

        std::string adItem = "RAWPEL=" + pelFilename.string();
        std::vector<std::string> additionalData{adItem};
        std::vector<std::string> associations;

        manager.create("error message", 42, 0,
                       phosphor::logging::Entry::Level::Error, additionalData,
                       associations);

        // Simulate the code getting back to the event loop
        // after each create.
        e.run(std::chrono::milliseconds(1));

        if (i < 24)
        {
            EXPECT_EQ(countPELsInRepo(), i);
        }
        else if (i == 24)
        {
            // Prune occured
            EXPECT_EQ(countPELsInRepo(), 7);
        }
        else // i == 25
        {
            EXPECT_EQ(countPELsInRepo(), 8);
        }
    }

    try
    {
        // Make sure the 8 newest ones are still found.
        for (uint32_t i = 0; i < 8; i++)
        {
            manager.getPEL(0x50000012 + i);
        }
    }
    catch (sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument& e)
    {
        ADD_FAILURE() << "PELs should have all been found";
    }

    fs::remove_all(dir);
}

// Test that manually deleting a PEL file will be recognized by the code.
TEST_F(ManagerTest, TestPELManualDelete)
{
    sdeventplus::Event e{sdEvent};

    std::unique_ptr<DataInterfaceBase> dataIface =
        std::make_unique<MockDataInterface>();

    openpower::pels::Manager manager{
        logManager, std::move(dataIface),
        std::bind(std::mem_fn(&TestLogger::log), &logger, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)};

    auto data = pelDataFactory(TestPELType::pelSimple);
    auto dir = makeTempDir();
    fs::path pelFilename = dir / "rawpel";

    std::string adItem = "RAWPEL=" + pelFilename.string();
    std::vector<std::string> additionalData{adItem};
    std::vector<std::string> associations;

    // Add 20 PELs, they will get incrementing IDs like
    // 0x50000001, 0x50000002, etc.
    for (int i = 1; i <= 20; i++)
    {
        std::ofstream pelFile{pelFilename};
        pelFile.write(reinterpret_cast<const char*>(data.data()), data.size());
        pelFile.close();

        manager.create("error message", 42, 0,
                       phosphor::logging::Entry::Level::Error, additionalData,
                       associations);

        // Sanity check this ID is really there so we can test
        // it was deleted later.  This will throw an exception if
        // not present.
        manager.getPEL(0x50000000 + i);

        // Run an event loop pass where the internal FD is deleted
        // after the getPEL function call.
        e.run(std::chrono::milliseconds(1));
    }

    EXPECT_EQ(countPELsInRepo(), 20);

    deletePELFile(0x50000001);

    // Run a single event loop pass so the inotify event can run
    e.run(std::chrono::milliseconds(1));

    EXPECT_EQ(countPELsInRepo(), 19);

    EXPECT_THROW(
        manager.getPEL(0x50000001),
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument);

    // Delete a few more, they should all get handled in the same
    // event loop pass
    std::vector<uint32_t> toDelete{0x50000002, 0x50000003, 0x50000004,
                                   0x50000005, 0x50000006};
    std::for_each(toDelete.begin(), toDelete.end(),
                  [](auto i) { deletePELFile(i); });

    e.run(std::chrono::milliseconds(1));

    EXPECT_EQ(countPELsInRepo(), 14);

    std::for_each(toDelete.begin(), toDelete.end(), [&manager](const auto i) {
        EXPECT_THROW(
            manager.getPEL(i),
            sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument);
    });

    fs::remove_all(dir);
}

// Test that deleting all PELs at once is handled OK.
TEST_F(ManagerTest, TestPELManualDeleteAll)
{
    sdeventplus::Event e{sdEvent};

    std::unique_ptr<DataInterfaceBase> dataIface =
        std::make_unique<MockDataInterface>();

    openpower::pels::Manager manager{
        logManager, std::move(dataIface),
        std::bind(std::mem_fn(&TestLogger::log), &logger, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)};

    auto data = pelDataFactory(TestPELType::pelSimple);
    auto dir = makeTempDir();
    fs::path pelFilename = dir / "rawpel";

    std::string adItem = "RAWPEL=" + pelFilename.string();
    std::vector<std::string> additionalData{adItem};
    std::vector<std::string> associations;

    // Add 200 PELs, they will get incrementing IDs like
    // 0x50000001, 0x50000002, etc.
    for (int i = 1; i <= 200; i++)
    {
        std::ofstream pelFile{pelFilename};
        pelFile.write(reinterpret_cast<const char*>(data.data()), data.size());
        pelFile.close();

        manager.create("error message", 42, 0,
                       phosphor::logging::Entry::Level::Error, additionalData,
                       associations);

        // Sanity check this ID is really there so we can test
        // it was deleted later.  This will throw an exception if
        // not present.
        manager.getPEL(0x50000000 + i);

        // Run an event loop pass where the internal FD is deleted
        // after the getPEL function call.
        e.run(std::chrono::milliseconds(1));
    }

    // Delete them all at once
    auto logPath = getPELRepoPath() / "logs";
    std::string cmd = "rm " + logPath.string() + "/*";
    system(cmd.c_str());

    EXPECT_EQ(countPELsInRepo(), 0);

    // It will take 5 event loop passes to process them all
    for (int i = 0; i < 5; i++)
    {
        e.run(std::chrono::milliseconds(1));
    }

    for (int i = 1; i <= 200; i++)
    {
        EXPECT_THROW(
            manager.getPEL(0x50000000 + i),
            sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument);
    }

    fs::remove_all(dir);
}
