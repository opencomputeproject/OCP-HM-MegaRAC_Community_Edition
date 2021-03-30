#include "writefrudata.hpp"

#include <CLI/CLI.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;

//--------------------------------------------------------------------------
// This gets called by udev monitor soon after seeing hog plugs for EEPROMS.
//--------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int rc = 0;
    uint8_t fruid;
    std::string eeprom_file;
    const int MAX_FRU_ID = 0xfe;

    CLI::App app{"OpenBMC IPMI-FRU-Parser"};
    app.add_option("-e,--eeprom", eeprom_file, "Absolute file name of eeprom")
        ->check(CLI::ExistingFile);
    app.add_option("-f,--fruid", fruid, "valid fru id in integer")
        ->check(CLI::Range(0, MAX_FRU_ID));

    // Read the arguments.
    CLI11_PARSE(app, argc, argv);

    // Now that we have the file that contains the eeprom data, go read it
    // and update the Inventory DB.
    auto bus = sdbusplus::bus::new_default();
    bool bmc_fru = true;
    rc = validateFRUArea(fruid, eeprom_file.c_str(), bus, bmc_fru);

    return (rc < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
