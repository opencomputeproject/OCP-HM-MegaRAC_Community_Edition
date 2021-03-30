#include "systemd_target_parser.hpp"
#include "systemd_target_signal.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <iostream>
#include <vector>

using phosphor::logging::level;
using phosphor::logging::log;

bool gVerbose = false;

void dump_targets(const TargetErrorData& targetData)
{
    std::cout << "## Data Structure of Json ##" << std::endl;
    for (const auto& [target, value] : targetData)
    {
        std::cout << target << " " << value.errorToLog << std::endl;
        std::cout << "    ";
        for (auto& eToMonitor : value.errorsToMonitor)
        {
            std::cout << eToMonitor << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void print_usage(void)
{
    std::cout << "[-f <file1> -f <file2> ...] : Full path to json file(s) with "
                 "target/error mappings"
              << std::endl;
    return;
}

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    std::vector<std::string> filePaths;

    CLI::App app{"OpenBmc systemd target monitor"};
    app.add_option("-f,--file", filePaths,
                   "Full path to json file(s) with target/error mappings");
    app.add_flag("-v", gVerbose, "Enable verbose output");

    CLI11_PARSE(app, argc, argv);

    if (filePaths.empty())
    {
        log<level::ERR>("No input files");
        print_usage();
        exit(-1);
    }

    TargetErrorData targetData = parseFiles(filePaths);

    if (targetData.size() == 0)
    {
        log<level::ERR>("Invalid input files, no targets found");
        print_usage();
        exit(-1);
    }

    if (gVerbose)
    {
        dump_targets(targetData);
    }

    phosphor::state::manager::SystemdTargetLogging targetMon(targetData, bus);

    // Subscribe to systemd D-bus signals indicating target completions
    targetMon.subscribeToSystemdSignals();

    return event.loop();
}
