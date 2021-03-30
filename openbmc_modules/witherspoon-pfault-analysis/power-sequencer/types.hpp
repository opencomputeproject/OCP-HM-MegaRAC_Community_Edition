#pragma once

#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace witherspoon
{
namespace power
{

class UCD90160;

namespace ucd90160
{

/**
 * Defines which extra analysis is required
 * on failures, if any.
 */
enum class extraAnalysisType
{
    none,
    gpuPGOOD,
    gpuOverTemp,
    memGOOD0,
    memGOOD1
};

/**
 * Options for the GPIOs
 *
 * Used as a bitmask
 */
enum class optionFlags
{
    none = 0,
    shutdownOnFault = 1
};

constexpr auto gpioNumField = 0;
constexpr auto gpioCalloutField = 1;
using GPIODefinition = std::tuple<gpio::gpioNum_t, std::string>;
using GPIODefinitions = std::vector<GPIODefinition>;

constexpr auto gpioDevicePathField = 0;
constexpr auto gpioPolarityField = 1;
constexpr auto errorFunctionField = 2;
constexpr auto optionFlagsField = 3;
constexpr auto gpioDefinitionField = 4;

using ErrorFunction = std::function<void(UCD90160&, const std::string&)>;

using GPIOGroup = std::tuple<std::string, gpio::Value, ErrorFunction,
                             optionFlags, GPIODefinitions>;

using GPIOAnalysis = std::map<extraAnalysisType, GPIOGroup>;

constexpr auto gpiNumField = 0;
constexpr auto pinIDField = 1;
constexpr auto gpiNameField = 2;
constexpr auto pollField = 3;
constexpr auto extraAnalysisField = 4;

using GPIConfig =
    std::tuple<size_t, size_t, std::string, bool, extraAnalysisType>;

using GPIConfigs = std::vector<GPIConfig>;

using RailNames = std::vector<std::string>;

constexpr auto pathField = 0;
constexpr auto railNamesField = 1;
constexpr auto gpiConfigField = 2;
constexpr auto gpioAnalysisField = 3;

using DeviceDefinition =
    std::tuple<std::string, RailNames, GPIConfigs, GPIOAnalysis>;

// Maps a device instance to its definition
using DeviceMap = std::map<size_t, DeviceDefinition>;

} // namespace ucd90160
} // namespace power
} // namespace witherspoon
