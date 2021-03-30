#pragma once

#include "device.hpp"
#include "gpio.hpp"
#include "pmbus.hpp"
#include "types.hpp"

#include <sdbusplus/bus.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <vector>

namespace witherspoon
{
namespace power
{

// Error type, callout
using PartCallout = std::tuple<ucd90160::extraAnalysisType, std::string>;

/**
 * @class UCD90160
 *
 * This class implements fault analysis for the UCD90160
 * power sequencer device.
 *
 */
class UCD90160 : public Device
{
  public:
    UCD90160() = delete;
    ~UCD90160() = default;
    UCD90160(const UCD90160&) = delete;
    UCD90160& operator=(const UCD90160&) = delete;
    UCD90160(UCD90160&&) = default;
    UCD90160& operator=(UCD90160&&) = default;

    /**
     * Constructor
     *
     * @param[in] instance - the device instance number
     * @param[in] bus - D-Bus bus object
     */
    UCD90160(size_t instance, sdbusplus::bus::bus& bus);

    /**
     * Analyzes the device for errors when the device is
     * known to be in an error state.  A log will be created.
     */
    void onFailure() override;

    /**
     * Checks the device for errors and only creates a log
     * if one is found.
     */
    void analyze() override;

    /**
     * Clears faults in the device
     */
    void clearFaults() override
    {}

  private:
    /**
     * Reports an error for a GPU PGOOD failure
     *
     * @param[in] callout - the GPU callout string
     */
    void gpuPGOODError(const std::string& callout);

    /**
     * Reports an error for a GPU OverTemp failure
     *
     * @param[in] callout - the GPU callout string
     */
    void gpuOverTempError(const std::string& callout);

    /**
     * Reports an error for a MEM_GOODx failure.
     *
     * @param[in] callout - The MEM callout string
     */
    void memGoodError(const std::string& callout);

    /**
     * Given the device path for a chip, find its gpiochip
     * path
     *
     * @param[in] path - device path, like
     *                   /sys/devices/.../i2c-11/11-0064
     *
     * @return fs::path - The gpiochip path, like
     *                   /dev/gpiochip1
     */
    static std::filesystem::path
        findGPIODevice(const std::filesystem::path& path);

    /**
     * Checks for VOUT faults on the device.
     *
     * This device can monitor voltages of its dependent
     * devices, and VOUT faults are voltage faults
     * on these devices.
     *
     * @return bool - true if an error log was created
     */
    bool checkVOUTFaults();

    /**
     * Checks for PGOOD faults on the device.
     *
     * This device can monitor the PGOOD signals of its dependent
     * devices, and this check will look for faults of
     * those PGOODs.
     *
     * @param[in] polling - If this is running while polling for errors,
     *                      as opposing to analyzing a fail condition.
     *
     * @return bool - true if an error log was created
     */
    bool checkPGOODFaults(bool polling);

    /**
     * Creates an error log when the device has an error
     * but it isn't a PGOOD or voltage failure.
     */
    void createPowerFaultLog();

    /**
     * Reads the status_word register
     *
     * @return uint16_t - the register contents
     */
    uint16_t readStatusWord();

    /**
     * Reads the mfr_status register
     *
     * @return uint32_t - the register contents
     */
    uint32_t readMFRStatus();

    /**
     * Does any additional fault analysis based on the
     * value of the extraAnalysisType field in the GPIOConfig
     * entry.
     *
     * Used to get better callouts.
     *
     * @param[in] config - the GPIOConfig entry to use
     *
     * @return bool - true if a HW error was found, false else
     */
    bool doExtraAnalysis(const ucd90160::GPIConfig& config);

    /**
     * Does additional fault analysis using GPIOs to
     * specifically identify the failing part.
     *
     * Used when there are too many PGOOD inputs for
     * the UCD90160 to handle, so just a summary bit
     * is wired into the chip, and then the specific
     * fault GPIOs are off of a different GPIO device,
     * like an IO expander.
     *
     * @param[in] type - the type of analysis to do
     *
     * @return bool - true if a HW error was found, false else
     */
    bool doGPIOAnalysis(ucd90160::extraAnalysisType type);

    /**
     * Says if we've already logged a Vout fault
     *
     * The policy is only 1 of the same error will
     * be logged for the duration of a class instance.
     *
     * @param[in] page - the page to check
     *
     * @return bool - if we've already logged a fault against
     *                this page
     */
    inline bool isVoutFaultLogged(uint32_t page) const
    {
        return std::find(voutErrors.begin(), voutErrors.end(), page) !=
               voutErrors.end();
    }

    /**
     * Saves that a Vout fault has been logged
     *
     * @param[in] page - the page the error was logged against
     */
    inline void setVoutFaultLogged(uint32_t page)
    {
        voutErrors.push_back(page);
    }

    /**
     * Says if we've already logged a PGOOD fault
     *
     * The policy is only 1 of the same errors will
     * be logged for the duration of a class instance.
     *
     * @param[in] input - the input to check
     *
     * @return bool - if we've already logged a fault against
     *                this input
     */
    inline bool isPGOODFaultLogged(uint32_t input) const
    {
        return std::find(pgoodErrors.begin(), pgoodErrors.end(), input) !=
               pgoodErrors.end();
    }

    /**
     * Says if we've already logged a specific fault
     * against a specific part
     *
     * @param[in] callout - error type and name tuple
     *
     * @return bool - if we've already logged this fault
     *                against this part
     */
    inline bool isPartCalledOut(const PartCallout& callout) const
    {
        return std::find(callouts.begin(), callouts.end(), callout) !=
               callouts.end();
    }

    /**
     * Saves that a PGOOD fault has been logged
     *
     * @param[in] input - the input the error was logged against
     */
    inline void setPGOODFaultLogged(uint32_t input)
    {
        pgoodErrors.push_back(input);
    }

    /**
     * Saves that a specific fault on a specific part has been done
     *
     * @param[in] callout - error type and name tuple
     */
    inline void setPartCallout(const PartCallout& callout)
    {
        callouts.push_back(callout);
    }

    /**
     * List of pages that Vout errors have
     * already been logged against
     */
    std::vector<uint32_t> voutErrors;

    /**
     * List of inputs that PGOOD errors have
     * already been logged against
     */
    std::vector<uint32_t> pgoodErrors;

    /**
     * List of callouts that already been done
     */
    std::vector<PartCallout> callouts;

    /**
     * The read/write interface to this hardware
     */
    pmbus::PMBus interface;

    /**
     * A map of GPI pin IDs to the GPIO object
     * used to access them
     */
    std::map<size_t, std::unique_ptr<gpio::GPIO>> gpios;

    /**
     * Keeps track of device access errors to avoid repeatedly
     * logging errors for bad hardware
     */
    bool accessError = false;

    /**
     * Keeps track of GPIO access errors when doing the in depth
     * PGOOD fault analysis to avoid repeatedly logging errors
     * for bad hardware
     */
    bool gpioAccessError = false;

    /**
     * The path to the GPIO device used to read
     * the GPI (PGOOD) status
     */
    std::filesystem::path gpioDevice;

    /**
     * The D-Bus bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * Map of device instance to the instance specific data
     */
    static const ucd90160::DeviceMap deviceMap;
};

} // namespace power
} // namespace witherspoon
