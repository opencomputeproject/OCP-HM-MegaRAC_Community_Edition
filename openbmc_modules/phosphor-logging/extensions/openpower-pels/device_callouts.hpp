#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>
#include <vector>

/**
 * @file device_callouts.hpp
 *
 * @brief Looks up FRU callouts, which are D-Bus inventory paths,
 * in JSON for device sysfs paths.
 *
 * The code will extract the search keys from the sysfs path to
 * use to look up the callout list in the JSON.  The callouts will
 * be sorted by priority as defined in th PEL spec.
 *
 * The JSON is normally generated from the MRW, and contains
 * sections for the following types of callouts:
 *   * I2C  (also based on bus/addr as well as the path)
 *   * FSI
 *   * FSI-I2C
 *   * FSI-SPI
 *
 * The JSON looks like:
 *
 * "I2C":
 *   "<bus>":
 *     "<address>":
 *       "Callouts": [
 *          {
 *             "LocationCode": "<location code>",
 *             "Name": "<inventory path>",
 *             "Priority": "<priority=H/M/L>",
 *             "MRU": "<optional MRU name>"
 *          },
 *          ...
 *       ],
 *       "Dest": "<destination MRW target>"
 *
 * "FSI":
 *   "<fsi link>":
 *     "Callouts": [
 *            ...
 *     ],
 *     "Dest": "<destination MRW target>"
 *
 * "FSI-I2C":
 *    "<fsi link>":
 *      "<bus>":
 *        "<address>":
 *            "Callouts": [
 *                   ...
 *             ],
 *            "Dest": "<destination MRW target>"
 *
 * "FSI-SPI":
 *    "<fsi link>":
 *      "<bus>":
 *         "Callouts": [
 *                ...
 *         ],
 *         "Dest": "<destination MRW target>"
 *
 */

namespace openpower::pels::device_callouts
{

/**
 * @brief Represents a callout in the device JSON.
 *
 * The debug field will only be filled in for the first
 * callout in the list of them and contains additional
 * information about what happened when looking up the
 * callouts that is meant to aid in debug.
 */
struct Callout
{
    std::string priority;
    std::string locationCode;
    std::string name;
    std::string mru;
    std::string debug;
};

/**
 * @brief Looks up the callouts in a JSON File to add to a PEL
 *        for when the path between the BMC and the device specified
 *        by the passed in device path needs to be called out.
 *
 * The path is the path used to access the device in sysfs.  It
 * can be either a directory path or a file path.
 *
 * @param[in] devPath - The device path
 * @param[in] compatibleList - The list of compatible names for this
 *                             system.
 * @return std::vector<Callout> - The list of callouts
 */
std::vector<Callout>
    getCallouts(const std::string& devPath,
                const std::vector<std::string>& compatibleList);

/**
 * @brief Looks up the callouts to add to a PEL for when the path
 *        between the BMC and the device specified by the passed in
 *        I2C bus and address needs to be called out.
 *
 * @param[in] i2cBus - The I2C bus
 * @param[in] i2cAddress - The I2C address
 * @param[in] compatibleList - The list of compatible names for this
 *                             system.
 * @return std::vector<Callout> - The list of callouts
 */
std::vector<Callout>
    getI2CCallouts(size_t i2cBus, uint8_t i2cAddress,
                   const std::vector<std::string>& compatibleList);

namespace util
{

/**
 * @brief The different callout path types
 */
enum class CalloutType
{
    i2c,
    fsi,
    fsii2c,
    fsispi,
    unknown
};

/**
 * @brief Returns the path to the JSON file to look up callouts in.
 *
 * @param[in] compatibleList - The list of compatible names for this
 *                             system.
 *
 * @return path - The path to the file.
 */
std::filesystem::path
    getJSONFilename(const std::vector<std::string>& compatibleList);

/**
 * @brief Looks up the callouts in the JSON using the I2C keys.
 *
 * @param[in] i2cBus - The I2C bus
 * @param[in] i2cAddress - The I2C address
 * @param[in] calloutJSON - The JSON containing the callouts
 *
 * @return std::vector<Callout> - The callouts
 */
std::vector<device_callouts::Callout>
    calloutI2C(size_t i2CBus, uint8_t i2cAddress,
               const nlohmann::json& calloutJSON);

/**
 * @brief Determines the type of the path (FSI, I2C, etc) based
 *        on tokens in the device path.
 *
 * @param[in] devPath - The device path
 *
 * @return CalloutType - The callout type
 */
CalloutType getCalloutType(const std::string& devPath);

/**
 * @brief Pulls the fields out of the I2C device path to use as search keys
 *        in the JSON.
 *
 * The keys are the I2C bus and address.
 *
 * @param[in] devPath - The device path
 *
 * @return std::tuple<size_t, uint8_t> - The I2C bus and address keys
 */
std::tuple<size_t, uint8_t> getI2CSearchKeys(const std::string& devPath);

/**
 * @brief Pulls the fields out of the FSI device path to use as search keys
 *        in the JSON.
 *
 * The key is the FSI link.  For multi-hop paths, the links are
 * separated by '-'s, like "0-1-2".
 *
 * @param[in] devPath - The device path
 *
 * @return std::string - The FSI links key
 */
std::string getFSISearchKeys(const std::string& devPath);

/**
 * @brief Pulls the fields out of the FSI-I2C device path to use as
 *        search keys in the JSON.
 *
 * The keys are the FSI link string and the  I2C bus and address.
 *
 * @param[in] devPath - The device path
 *
 * @return std::tuple<std::string, std::tuple<size_t, uint8_t>>
 *         - The FSI links key along with the I2C bus/address.
 */
std::tuple<std::string, std::tuple<size_t, uint8_t>>
    getFSII2CSearchKeys(const std::string& devPath);

/**
 * @brief Pulls the fields out of the SPI device path to use as search keys
 *        in the JSON.
 *
 * The key is the SPI bus number.
 *
 * @param[in] devPath - The device path
 *
 * @return size_t - The SPI bus key
 */
size_t getSPISearchKeys(const std::string& devPath);

/**
 * @brief Pulls the fields out of the FSI-SPI device path to use as
 *        search keys in the JSON.
 *
 * The keys are the FSI link string and the SPI bus number.
 *
 * @param[in] devPath - The device path
 *
 * @return std::tuple<std::string, size_t>
 *         - The FSI links key along with the SPI bus number.
 */
std::tuple<std::string, size_t> getFSISPISearchKeys(const std::string& devPath);

} // namespace util
} // namespace openpower::pels::device_callouts
