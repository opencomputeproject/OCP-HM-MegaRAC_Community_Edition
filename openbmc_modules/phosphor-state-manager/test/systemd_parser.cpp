#include <systemd_target_parser.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

// Enable debug by default for debug when needed
bool gVerbose = true;

TEST(TargetJsonParser, BasicGoodPath)
{
    auto defaultData1 = R"(
        {
            "targets" : {
                "multi-user.target" : {
                    "errorsToMonitor": ["default"],
                    "errorToLog": "xyz.openbmc_project.State.BMC.Error.MultiUserTargetFailure"},
                "obmc-chassis-poweron@0.target" : {
                    "errorsToMonitor": ["timeout", "failed"],
                    "errorToLog": "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure"}
                }
        }
    )"_json;

    auto defaultData2 = R"(
        {
            "targets" : {
                "obmc-host-start@0.target" : {
                    "errorsToMonitor": ["default"],
                    "errorToLog": "xyz.openbmc_project.State.Host.Error.HostStartFailure"},
                "obmc-host-stop@0.target" : {
                    "errorsToMonitor": ["dependency"],
                    "errorToLog": "xyz.openbmc_project.State.Host.Error.HostStopFailure"}
                }
        }
    )"_json;

    std::FILE* tmpf = fopen("/tmp/good_file1.json", "w");
    std::fputs(defaultData1.dump().c_str(), tmpf);
    std::fclose(tmpf);

    tmpf = fopen("/tmp/good_file2.json", "w");
    std::fputs(defaultData2.dump().c_str(), tmpf);
    std::fclose(tmpf);

    std::vector<std::string> filePaths;
    filePaths.push_back("/tmp/good_file1.json");
    filePaths.push_back("/tmp/good_file2.json");

    TargetErrorData targetData = parseFiles(filePaths);

    EXPECT_EQ(targetData.size(), 4);
    EXPECT_NE(targetData.find("multi-user.target"), targetData.end());
    EXPECT_NE(targetData.find("obmc-chassis-poweron@0.target"),
              targetData.end());
    EXPECT_NE(targetData.find("obmc-host-start@0.target"), targetData.end());
    EXPECT_NE(targetData.find("obmc-host-stop@0.target"), targetData.end());
    targetEntry tgt = targetData["obmc-chassis-poweron@0.target"];
    EXPECT_EQ(tgt.errorToLog,
              "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure");
    EXPECT_EQ(tgt.errorsToMonitor.size(), 2);
    // Check a target with "default" for errorsToMonitor, should have 3 defaults
    tgt = targetData["obmc-host-start@0.target"];
    EXPECT_EQ(tgt.errorsToMonitor.size(), 3);

    std::remove("/tmp/good_file1.json");
    std::remove("/tmp/good_file2.json");
}

TEST(TargetJsonParser, InvalidErrorToMonitor)
{
    auto invalidDataError = R"(
        {
            "targets" : {
                "obmc-chassis-poweron@0.target" : {
                    "errorsToMonitor": ["timeout", "invalid"],
                    "errorToLog": "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure"}
                }
        }
    )"_json;

    std::FILE* tmpf = fopen("/tmp/invalid_error_file.json", "w");
    std::fputs(invalidDataError.dump().c_str(), tmpf);
    std::fclose(tmpf);

    std::vector<std::string> filePaths;
    filePaths.push_back("/tmp/invalid_error_file.json");

    // Verify exception thrown on invalid errorsToMonitor
    EXPECT_THROW(TargetErrorData targetData = parseFiles(filePaths),
                 std::out_of_range);
    std::remove("/tmp/invalid_error_file.json");
}

TEST(TargetJsonParser, InvalidFileFormat)
{
    std::FILE* tmpf = fopen("/tmp/invalid_json_file.json", "w");
    std::fputs("{\"targets\":{\"missing closing quote}}", tmpf);
    fclose(tmpf);

    std::vector<std::string> filePaths;
    filePaths.push_back("/tmp/invalid_json_file.json");

    // Verify exception thrown on invalid json file format
    EXPECT_THROW(TargetErrorData targetData = parseFiles(filePaths),
                 nlohmann::detail::parse_error);
    std::remove("/tmp/invalid_json_file.json");
}

TEST(TargetJsonParser, NotJustDefault)
{
    auto notJustDefault = R"(
        {
            "targets" : {
                "obmc-chassis-poweron@0.target" : {
                    "errorsToMonitor": ["timeout", "default"],
                    "errorToLog": "xyz.openbmc_project.State.Chassis.Error.PowerOnTargetFailure"}
                }
        }
    )"_json;

    std::FILE* tmpf = fopen("/tmp/not_just_default_file.json", "w");
    std::fputs(notJustDefault.dump().c_str(), tmpf);
    std::fclose(tmpf);

    std::vector<std::string> filePaths;
    filePaths.push_back("/tmp/not_just_default_file.json");

    // Verify exception thrown on invalid errorsToMonitor
    EXPECT_THROW(TargetErrorData targetData = parseFiles(filePaths),
                 std::invalid_argument);
    std::remove("/tmp/not_just_default_file.json");
}
