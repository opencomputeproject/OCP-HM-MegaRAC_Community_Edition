#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace witherspoon
{
namespace pmbus
{

namespace fs = std::filesystem;

// The file name Linux uses to capture the STATUS_WORD from pmbus.
constexpr auto STATUS_WORD = "status0";

// The file name Linux uses to capture the STATUS_INPUT from pmbus.
constexpr auto STATUS_INPUT = "status0_input";

// Voltage out status.
// Overvoltage fault or warning, Undervoltage fault or warning, maximum or
// minimum warning, ....
// Uses Page substitution
constexpr auto STATUS_VOUT = "statusP_vout";

namespace status_vout
{
// Mask of bits that are only warnings
constexpr auto WARNING_MASK = 0x6A;
} // namespace status_vout

// Current output status bits.
constexpr auto STATUS_IOUT = "status0_iout";

// Manufacturing specific status bits
constexpr auto STATUS_MFR = "status0_mfr";

// Reports on the status of any fans installed in position 1 and 2.
constexpr auto STATUS_FANS_1_2 = "status0_fans12";

// Reports on temperature faults or warnings. Overtemperature fault,
// overtemperature warning, undertemperature warning, undertemperature fault.
constexpr auto STATUS_TEMPERATURE = "status0_temp";

namespace status_word
{
constexpr auto VOUT_FAULT = 0x8000;

// The IBM CFF power supply driver does map this bit to power1_alarm in the
// hwmon space, but since the other bits that need to be checked do not have
// a similar mapping, the code will just read STATUS_WORD and use bit masking
// to see if the INPUT FAULT OR WARNING bit is on.
constexpr auto INPUT_FAULT_WARN = 0x2000;

// The bit mask representing the POWER_GOOD Negated bit of the STATUS_WORD.
constexpr auto POWER_GOOD_NEGATED = 0x0800;

// The bit mask representing the FAN FAULT or WARNING bit of the STATUS_WORD.
// Bit 2 of the high byte of STATUS_WORD.
constexpr auto FAN_FAULT = 0x0400;

// The bit mask representing the UNITI_IS_OFF bit of the STATUS_WORD.
constexpr auto UNIT_IS_OFF = 0x0040;

// Bit 5 of the STATUS_BYTE, or lower byte of STATUS_WORD is used to indicate
// an output overvoltage fault.
constexpr auto VOUT_OV_FAULT = 0x0020;

// The bit mask representing that an output overcurrent fault has occurred.
constexpr auto IOUT_OC_FAULT = 0x0010;

// The IBM CFF power supply driver does map this bit to in1_alarm, however,
// since a number of the other bits are not mapped that way for STATUS_WORD,
// this code will just read the entire STATUS_WORD and use bit masking to find
// out if that fault is on.
constexpr auto VIN_UV_FAULT = 0x0008;

// The bit mask representing the TEMPERATURE FAULT or WARNING bit of the
// STATUS_WORD. Bit 2 of the low byte (STATUS_BYTE).
constexpr auto TEMPERATURE_FAULT_WARN = 0x0004;

} // namespace status_word

namespace status_temperature
{
// Overtemperature Fault
constexpr auto OT_FAULT = 0x80;
} // namespace status_temperature

/**
 * Where the access should be done
 */
enum class Type
{
    Base,            // base device directory
    Hwmon,           // hwmon directory
    Debug,           // pmbus debug directory
    DeviceDebug,     // device debug directory
    HwmonDeviceDebug // hwmon device debug directory
};

/**
 * @class PMBus
 *
 * This class is an interface to communicating with PMBus devices
 * by reading and writing sysfs files.
 *
 * Based on the Type parameter, the accesses can either be done
 * in the base device directory (the one passed into the constructor),
 * or in the hwmon directory for the device.
 */
class PMBus
{
  public:
    PMBus() = delete;
    ~PMBus() = default;
    PMBus(const PMBus&) = default;
    PMBus& operator=(const PMBus&) = default;
    PMBus(PMBus&&) = default;
    PMBus& operator=(PMBus&&) = default;

    /**
     * Constructor
     *
     * @param[in] path - path to the sysfs directory
     */
    PMBus(const std::string& path) : basePath(path)
    {
        findHwmonDir();
    }

    /**
     * Constructor
     *
     * This version is required when DeviceDebug
     * access will be used.
     *
     * @param[in] path - path to the sysfs directory
     * @param[in] driverName - the device driver name
     * @param[in] instance - chip instance number
     */
    PMBus(const std::string& path, const std::string& driverName,
          size_t instance) :
        basePath(path),
        driverName(driverName), instance(instance)
    {
        findHwmonDir();
    }

    /**
     * Reads a file in sysfs that represents a single bit,
     * therefore doing a PMBus read.
     *
     * @param[in] name - path concatenated to
     *                   basePath to read
     * @param[in] type - Path type
     *
     * @return bool - false if result was 0, else true
     */
    bool readBit(const std::string& name, Type type);

    /**
     * Reads a file in sysfs that represents a single bit,
     * where the page number passed in is substituted
     * into the name in place of the 'P' character in it.
     *
     * @param[in] name - path concatenated to
     *                   basePath to read
     * @param[in] page - page number
     * @param[in] type - Path type
     *
     * @return bool - false if result was 0, else true
     */
    bool readBitInPage(const std::string& name, size_t page, Type type);
    /**
     * Checks if the file for the given name and type exists.
     *
     * @param[in] name   - path concatenated to basePath to read
     * @param[in] type   - Path type
     *
     * @return bool - True if file exists, false if it does not.
     */
    bool exists(const std::string& name, Type type);

    /**
     * Read byte(s) from file in sysfs.
     *
     * @param[in] name   - path concatenated to basePath to read
     * @param[in] type   - Path type
     *
     * @return uint64_t - Up to 8 bytes of data read from file.
     */
    uint64_t read(const std::string& name, Type type);

    /**
     * Read a string from file in sysfs.
     *
     * @param[in] name   - path concatenated to basePath to read
     * @param[in] type   - Path type
     *
     * @return string - The data read from the file.
     */
    std::string readString(const std::string& name, Type type);

    /**
     * Read data from a binary file in sysfs.
     *
     * @param[in] name   - path concatenated to basePath to read
     * @param[in] type   - Path type
     * @param[in] length - length of data to read, in bytes
     *
     * @return vector<uint8_t> - The data read from the file.
     */
    std::vector<uint8_t> readBinary(const std::string& name, Type type,
                                    size_t length);

    /**
     * Writes an integer value to the file, therefore doing
     * a PMBus write.
     *
     * @param[in] name - path concatenated to
     *                   basePath to write
     * @param[in] value - the value to write
     * @param[in] type - Path type
     */
    void write(const std::string& name, int value, Type type);

    /**
     * Returns the sysfs base path of this device
     */
    inline const auto& path() const
    {
        return basePath;
    }

    /**
     * Replaces the 'P' in the string passed in with
     * the page number passed in.
     *
     * For example:
     *   insertPageNum("inP_enable", 42)
     *   returns "in42_enable"
     *
     * @param[in] templateName - the name string, with a 'P' in it
     * @param[in] page - the page number to insert where the P was
     *
     * @return string - the new string with the page number in it
     */
    static std::string insertPageNum(const std::string& templateName,
                                     size_t page);

    /**
     * Finds the path relative to basePath to the hwmon directory
     * for the device and stores it in hwmonRelPath.
     */
    void findHwmonDir();

    /**
     * Returns the path to use for the passed in type.
     *
     * @param[in] type - Path type
     *
     * @return fs::path - the full path
     */
    fs::path getPath(Type type);

  private:
    /**
     * Returns the device name
     *
     * This is found in the 'name' file in basePath.
     *
     * @return string - the device name
     */
    std::string getDeviceName();

    /**
     * The sysfs device path
     */
    fs::path basePath;

    /**
     * The directory name under the basePath hwmon directory
     */
    fs::path hwmonDir;

    /**
     * The device driver name.  Used for finding the device
     * debug directory.  Not required if that directory
     * isn't used.
     */
    std::string driverName;

    /**
     * The device instance number.
     *
     * Used in conjunction with the driver name for finding
     * the debug directory.  Not required if that directory
     * isn't used.
     */
    size_t instance = 0;

    /**
     * The pmbus debug path with status files
     */
    const fs::path debugPath = "/sys/kernel/debug/";
};

} // namespace pmbus
} // namespace witherspoon
