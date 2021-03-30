#pragma once

#include "interface.hpp"

#include <string>
#include <tuple>

namespace hwmon
{
namespace entry
{
static constexpr auto cinput = "input";
static constexpr auto clabel = "label";
static constexpr auto ctarget = "target";
static constexpr auto cenable = "enable";
static constexpr auto cfault = "fault";
static constexpr auto caverage = "average";
static constexpr auto caverage_interval = "average_interval";

static const std::string input = cinput;
static const std::string label = clabel;
static const std::string target = ctarget;
static const std::string enable = cenable;
static const std::string fault = cfault;
static const std::string average = caverage;
static const std::string average_interval = caverage_interval;
} // namespace entry

namespace type
{
static constexpr auto cfan = "fan";
static constexpr auto ctemp = "temp";
static constexpr auto cvolt = "in";
static constexpr auto ccurr = "curr";
static constexpr auto cenergy = "energy";
static constexpr auto cpower = "power";
static constexpr auto cpwm = "pwm";

static const std::string fan = cfan;
static const std::string temp = ctemp;
static const std::string volt = cvolt;
static const std::string curr = ccurr;
static const std::string energy = cenergy;
static const std::string power = cpower;
static const std::string pwm = cpwm;
} // namespace type

static constexpr auto typeAttrMap = {
    // 1 - hwmon class
    // 2 - unit
    // 3 - sysfs scaling factor
    // 4 - namespace
    std::make_tuple(hwmon::type::ctemp, ValueInterface::Unit::DegreesC, -3,
                    "temperature"),
    std::make_tuple(hwmon::type::cfan, ValueInterface::Unit::RPMS, 0,
                    "fan_tach"),
    std::make_tuple(hwmon::type::cvolt, ValueInterface::Unit::Volts, -3,
                    "voltage"),
    std::make_tuple(hwmon::type::ccurr, ValueInterface::Unit::Amperes, -3,
                    "current"),
    std::make_tuple(hwmon::type::cenergy, ValueInterface::Unit::Joules, -6,
                    "energy"),
    std::make_tuple(hwmon::type::cpower, ValueInterface::Unit::Watts, -6,
                    "power"),
};

inline auto getHwmonType(decltype(typeAttrMap)::const_reference attrs)
{
    return std::get<0>(attrs);
}

inline auto getUnit(decltype(typeAttrMap)::const_reference attrs)
{
    return std::get<1>(attrs);
}

inline auto getScale(decltype(typeAttrMap)::const_reference attrs)
{
    return std::get<2>(attrs);
}

inline auto getNamespace(decltype(typeAttrMap)::const_reference attrs)
{
    return std::get<3>(attrs);
}

using AttributeIterator = decltype(*typeAttrMap.begin());
using Attributes =
    std::remove_cv<std::remove_reference<AttributeIterator>::type>::type;

/** @brief Get Attribute tuple for the type
 *
 *  Given a type, it tries to find the corresponding tuple
 *
 *  @param[in] type the sensor type
 *  @param[in,out] A pointer to the Attribute tuple
 */
bool getAttributes(const std::string& type, Attributes& attributes);

} //  namespace hwmon

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
