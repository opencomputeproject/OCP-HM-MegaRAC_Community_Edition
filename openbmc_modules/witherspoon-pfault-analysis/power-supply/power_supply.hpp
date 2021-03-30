#pragma once
#include "average.hpp"
#include "device.hpp"
#include "maximum.hpp"
#include "names_values.hpp"
#include "pmbus.hpp"
#include "record_manager.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

namespace witherspoon
{
namespace power
{
namespace psu
{

namespace sdbusRule = sdbusplus::bus::match::rules;

constexpr auto FAULT_COUNT = 3;

/**
 * @class PowerSupply
 * Represents a PMBus power supply device.
 */
class PowerSupply : public Device
{
  public:
    PowerSupply() = delete;
    PowerSupply(const PowerSupply&) = delete;
    PowerSupply(PowerSupply&&) = default;
    PowerSupply& operator=(const PowerSupply&) = default;
    PowerSupply& operator=(PowerSupply&&) = default;
    ~PowerSupply() = default;

    /**
     * Constructor
     *
     * @param[in] name - the device name
     * @param[in] inst - the device instance
     * @param[in] objpath - the path to monitor
     * @param[in] invpath - the inventory path to use
     * @param[in] bus - D-Bus bus object
     * @param[in] e - event object
     * @param[in] t - time to allow power supply to assert PG#
     * @param[in] p - time to allow power supply presence state to
     *                settle/deglitch and allow for application of power
     *                prior to fault checking
     */
    PowerSupply(const std::string& name, size_t inst,
                const std::string& objpath, const std::string& invpath,
                sdbusplus::bus::bus& bus, const sdeventplus::Event& e,
                std::chrono::seconds& t, std::chrono::seconds& p);

    /**
     * Power supply specific function to analyze for faults/errors.
     *
     * Various PMBus status bits will be checked for fault conditions.
     * If a certain fault bits are on, the appropriate error will be
     * committed.
     */
    void analyze() override;

    /**
     * Write PMBus CLEAR_FAULTS
     *
     * This function will be called in various situations in order to clear
     * any fault status bits that may have been set, in order to start over
     * with a clean state. Presence changes and power state changes will
     * want to clear any faults logged.
     */
    void clearFaults() override;

    /**
     * Mark error for specified callout and message as resolved.
     *
     * @param[in] callout - The callout to be resolved (inventory path)
     * @parma[in] message - The message for the fault to be resolved
     */
    void resolveError(const std::string& callout, const std::string& message);

    /**
     * Enables making the input power history available on D-Bus
     *
     * @param[in] objectPath - the D-Bus object path to use
     * @param[in] maxRecords - the number of history records to keep
     * @param[in] syncGPIOPath - The gpiochip device path to use for
     *                           sending the sync command
     * @paramp[in] syncGPIONum - the GPIO number for the sync command
     */
    void enableHistory(const std::string& objectPath, size_t numRecords,
                       const std::string& syncGPIOPath, size_t syncGPIONum);

  private:
    /**
     * The path to use for reading various PMBus bits/words.
     */
    std::string monitorPath;

    /**
     * @brief Pointer to the PMBus interface
     *
     * Used to read out of or write to the /sysfs tree(s) containing files
     * that a device driver monitors the PMBus interface to the power
     * supplies.
     */
    witherspoon::pmbus::PMBus pmbusIntf;

    /**
     * @brief D-Bus path to use for this power supply's inventory status.
     */
    std::string inventoryPath;

    /** @brief Connection for sdbusplus bus */
    sdbusplus::bus::bus& bus;

    /** @brief True if the power supply is present. */
    bool present = false;

    /** @brief Used to subscribe to D-Bus property changes for Present */
    std::unique_ptr<sdbusplus::bus::match_t> presentMatch;

    /**
     * @brief Interval for setting present to true.
     *
     * The amount of time to wait from not present to present change before
     * updating the internal present indicator. Allows person servicing
     * the power supply some time to plug in the cable.
     */
    std::chrono::seconds presentInterval;

    /**
     * @brief Timer used to delay setting the internal present state.
     *
     * The timer used to do the callback after the present property has
     * changed.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> presentTimer;

    /** @brief True if a fault has already been found and not cleared */
    bool faultFound = false;

    /** @brief True if the power is on. */
    bool powerOn = false;

    /**
     * @brief Equal to FAULT_COUNT if power on fault has been
     * detected.
     */
    size_t powerOnFault = 0;

    /**
     * @brief Interval to setting powerOn to true.
     *
     * The amount of time to wait from power state on to setting the
     * internal powerOn state to true. The amount of time the power supply
     * is allowed to delay setting DGood/PG#.
     */
    std::chrono::seconds powerOnInterval;

    /**
     * @brief Timer used to delay setting the internal powerOn state.
     *
     * The timer used to do the callback after the power state has been on
     * long enough.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> powerOnTimer;

    /** @brief Used to subscribe to D-Bus power on state changes */
    std::unique_ptr<sdbusplus::bus::match_t> powerOnMatch;

    /** @brief Indicates that a read failure has occurred.
     *
     * @details This will be incremented each time a read failure is
     *          encountered. If it is incremented to FAULT_COUNT, an error
     *          will be logged.
     */
    size_t readFail = 0;

    /** @brief Has a PMBus read failure already been logged? */
    bool readFailLogged = false;

    /**
     * @brief Indicates an input fault or warning if equal to FAULT_COUNT.
     *
     * @details This is the "INPUT FAULT OR WARNING" bit in the high byte,
     *          or the VIN_UV_FAULT bit in the low byte in the STATUS_WORD
     *          command response. If either of those bits are on, this will
     *          be incremented.
     */
    size_t inputFault = 0;

    /**
     * @brief Indicates output over current fault if equal to FAULT_COUNT
     *
     * @details This is incremented when the "IOUT_OC_FAULT" bit in the low
     *          byte from the STATUS_WORD command response is on.
     */
    size_t outputOCFault = 0;

    /**
     * @brief Indicates output overvoltage fault if equal to FAULT_COUNT.
     *
     * @details This is incremented when the "VOUT_OV_FAULT" bit in the
     *          STATUS_WORD command response is on.
     */
    size_t outputOVFault = 0;

    /**
     * @brief Indicates a fan fault or warning condition was detected if
     *        equal to FAULT_COUNT.
     *
     * @details This is incremented when the 'FAN_FAULT' bit in the
     *          STATUS_WORD command response is on.
     */
    size_t fanFault = 0;

    /**
     * @brief Indicates a temperature fault or warn condition was detected
     *        if equal to FAULT_COUNT.
     *
     * @details This is incremented when the 'TEMPERATURE_FAULT_WARN' bit
     *          in the STATUS_WORD command response is on, or if the
     *          'OT_FAULT' bit in the STATUS_TEMPERATURE command response
     *          is on.
     */
    size_t temperatureFault = 0;

    /**
     * @brief Class that manages the input power history records.
     */
    std::unique_ptr<history::RecordManager> recordManager;

    /**
     * @brief The D-Bus object for the average input power history
     */
    std::unique_ptr<history::Average> average;

    /**
     * @brief The D-Bus object for the maximum input power history
     */
    std::unique_ptr<history::Maximum> maximum;

    /**
     * @brief The base D-Bus object path to use for the average
     *        and maximum objects.
     */
    std::string historyObjectPath;

    /**
     * @brief The GPIO device path to use for sending the 'sync'
     *        command to the PS.
     */
    std::string syncGPIODevPath;

    /**
     * @brief The GPIO number to use for sending the 'sync'
     *        command to the PS.
     */
    size_t syncGPIONumber = 0;

    /**
     * @brief Callback for inventory property changes
     *
     * Process change of Present property for power supply.
     *
     * @param[in]  msg - Data associated with Present change signal
     *
     */
    void inventoryChanged(sdbusplus::message::message& msg);

    /**
     * Updates the presence status by querying D-Bus
     *
     * The D-Bus inventory properties for this power supply will be read to
     * determine if the power supply is present or not and update this
     * objects present member variable to reflect current status.
     */
    void updatePresence();

    /**
     * @brief Updates the poweredOn status by querying D-Bus
     *
     * The D-Bus property for the system power state will be read to
     * determine if the system is powered on or not.
     */
    void updatePowerState();

    /**
     * @brief Callback for power state property changes
     *
     * Process changes to the powered on stat property for the system.
     *
     * @param[in] msg - Data associated with the power state signal
     */
    void powerStateChanged(sdbusplus::message::message& msg);

    /**
     * @brief Wrapper for PMBus::read() and adding metadata
     *
     * @param[out] nv - NamesValues instance to store cmd string and value
     * @param[in] cmd - String for the command to read data from.
     * @param[in] type - The type of file to read the command from.
     */
    void captureCmd(util::NamesValues& nv, const std::string& cmd,
                    witherspoon::pmbus::Type type);

    /**
     * @brief Checks for input voltage faults and logs error if needed.
     *
     * Check for voltage input under voltage fault (VIN_UV_FAULT) and/or
     * input fault or warning (INPUT_FAULT), and logs appropriate error(s).
     *
     * @param[in] statusWord  - 2 byte STATUS_WORD value read from sysfs
     */
    void checkInputFault(const uint16_t statusWord);

    /**
     * @brief Checks for power good negated or unit is off in wrong state
     *
     * @param[in] statusWord  - 2 byte STATUS_WORD value read from sysfs
     */
    void checkPGOrUnitOffFault(const uint16_t statusWord);

    /**
     * @brief Checks for output current over current fault.
     *
     * IOUT_OC_FAULT is checked, if on, appropriate error is logged.
     *
     * @param[in] statusWord  - 2 byte STATUS_WORD value read from sysfs
     */
    void checkCurrentOutOverCurrentFault(const uint16_t statusWord);

    /**
     * @brief Checks for output overvoltage fault.
     *
     * VOUT_OV_FAULT is checked, if on, appropriate error is logged.
     *
     * @param[in] statusWord  - 2 byte STATUS_WORD value read from sysfs
     */
    void checkOutputOvervoltageFault(const uint16_t statusWord);

    /**
     * @brief Checks for a fan fault or warning condition.
     *
     * The high byte of STATUS_WORD is checked to see if the "FAN FAULT OR
     * WARNING" bit is turned on. If it is on, log an error.
     *
     * @param[in] statusWord - 2 byte STATUS_WORD value read from sysfs
     */
    void checkFanFault(const uint16_t statusWord);

    /**
     * @brief Checks for a temperature fault or warning condition.
     *
     * The low byte of STATUS_WORD is checked to see if the "TEMPERATURE
     * FAULT OR WARNING" bit is turned on. If it is on, log an error,
     * call out the power supply indicating the fault/warning condition.
     *
     * @parma[in] statusWord - 2 byte STATUS_WORD value read from sysfs
     */
    void checkTemperatureFault(const uint16_t statusWord);

    /**
     * @brief Adds properties to the inventory.
     *
     * Reads the values from the device and writes them to the
     * associated power supply D-Bus inventory object.
     *
     * This needs to be done on startup, and each time the presence
     * state changes.
     *
     * Properties added:
     * - Serial Number
     * - Part Number
     * - CCIN (Customer Card Identification Number) - added as the Model
     * - Firmware version
     */
    void updateInventory();

    /**
     * @brief Toggles the GPIO to sync power supply input history readings
     *
     * This GPIO is connected to all supplies.  This will clear the
     * previous readings out of the supplies and restart them both at the
     * same time zero and at record ID 0.  The supplies will return 0
     * bytes of data for the input history command right after this until
     * a new entry shows up.
     *
     * This will cause the code to delete all previous history data and
     * start fresh.
     */
    void syncHistory();

    /**
     * @brief Reads the most recent input history record from the power
     *        supply and updates the average and maximum properties in
     *        D-Bus if there is a new reading available.
     *
     * This will still run every time analyze() is called so code can
     * post new data as soon as possible and the timestamp will more
     * accurately reflect the correct time.
     *
     * D-Bus is only updated if there is a change and the oldest record
     * will be pruned if the property already contains the max number of
     * records.
     */
    void updateHistory();
};

} // namespace psu
} // namespace power
} // namespace witherspoon
