#include "config.h"

#include "elog_entry.hpp"
#include "elog_serialize.hpp"
#include "log_manager.hpp"

#include <filesystem>
#include <thread>

#include <gtest/gtest.h>

namespace phosphor
{
namespace logging
{
namespace test
{

using namespace std::chrono_literals;
namespace fs = std::filesystem;

// Test that the update timestamp changes when the resolved property changes
TEST(TestUpdateTS, testChangeResolved)
{
    // Setting resolved will serialize, so need this directory.
    fs::create_directory(ERRLOG_PERSIST_PATH);

    if (!fs::exists(ERRLOG_PERSIST_PATH))
    {
        ADD_FAILURE() << "Could not create " << ERRLOG_PERSIST_PATH << "\n";
        exit(1);
    }

    auto bus = sdbusplus::bus::new_default();
    phosphor::logging::internal::Manager manager(bus, OBJ_INTERNAL);

    // Use a random number for the ID to avoid other CI
    // testcases running in parallel.
    std::srand(std::time(nullptr));
    uint32_t id = std::rand();

    if (fs::exists(fs::path{ERRLOG_PERSIST_PATH} / std::to_string(id)))
    {
        std::cerr << "Another testcase is using ID " << id << "\n";
        id = std::rand();
    }

    uint64_t timestamp{100};
    std::string message{"test error"};
    std::string fwLevel{"level42"};
    std::vector<std::string> testData{"additional", "data"};
    phosphor::logging::AssociationList associations{};

    Entry elog{bus,
               std::string(OBJ_ENTRY) + '/' + std::to_string(id),
               id,
               timestamp,
               Entry::Level::Informational,
               std::move(message),
               std::move(testData),
               std::move(associations),
               fwLevel,
               manager};

    EXPECT_EQ(elog.timestamp(), elog.updateTimestamp());

    std::this_thread::sleep_for(1ms);

    elog.resolved(true);
    auto updateTS = elog.updateTimestamp();
    EXPECT_NE(updateTS, elog.timestamp());

    std::this_thread::sleep_for(1ms);

    elog.resolved(false);
    EXPECT_NE(updateTS, elog.updateTimestamp());
    updateTS = elog.updateTimestamp();

    std::this_thread::sleep_for(1ms);

    // No change
    elog.resolved(false);
    EXPECT_EQ(updateTS, elog.updateTimestamp());

    // Leave the directory in case other CI instances are running
    fs::remove(fs::path{ERRLOG_PERSIST_PATH} / std::to_string(id));
}

} // namespace test
} // namespace logging
} // namespace phosphor
