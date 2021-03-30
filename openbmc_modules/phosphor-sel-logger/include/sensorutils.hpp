/*
// Copyright (c) 2017 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once
#include <phosphor-logging/log.hpp>

#include <cmath>
#include <iostream>

namespace ipmi
{
/** @struct VariantToDoubleVisitor
 *  @brief Visitor to convert variants to doubles
 *  @details Performs a static cast on the underlying type
 */
struct VariantToDoubleVisitor
{
    template <typename T>
    double operator()(const T& t) const
    {
        static_assert(std::is_arithmetic_v<T>,
                      "Cannot translate type to double");
        return static_cast<double>(t);
    }
};

static constexpr int16_t maxInt10 = 0x1FF;
static constexpr int16_t minInt10 = -0x200;
static constexpr int8_t maxInt4 = 7;
static constexpr int8_t minInt4 = -8;

static inline bool getSensorAttributes(const double max, const double min,
                                       int16_t& mValue, int8_t& rExp,
                                       int16_t& bValue, int8_t& bExp,
                                       bool& bSigned)
{
    // computing y = (10^rRexp) * (Mx + (B*(10^Bexp)))
    // check for 0, assume always positive
    double mDouble;
    double bDouble;
    if (max <= min)
    {
        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            "getSensorAttributes: Max must be greater than min");
        return false;
    }

    mDouble = (max - min) / 0xFF;

    if (min < 0)
    {
        bSigned = true;
        bDouble = floor(0.5 + ((max + min) / 2));
    }
    else
    {
        bSigned = false;
        bDouble = min;
    }

    rExp = 0;

    // M too big for 10 bit variable
    while (mDouble > maxInt10)
    {
        if (rExp >= maxInt4)
        {
            phosphor::logging::log<phosphor::logging::level::DEBUG>(
                "rExp Too big, Max and Min range too far",
                phosphor::logging::entry("REXP=%d", rExp));
            return false;
        }
        mDouble /= 10;
        rExp++;
    }

    // M too small, loop until we lose less than 1 eight bit count of precision
    while (((mDouble - floor(mDouble)) / mDouble) > (1.0 / 255))
    {
        if (rExp <= minInt4)
        {
            phosphor::logging::log<phosphor::logging::level::DEBUG>(
                "rExp Too Small, Max and Min range too close");
            return false;
        }
        // check to see if we reached the limit of where we can adjust back the
        // B value
        if (bDouble / std::pow(10, rExp + minInt4 - 1) > bDouble)
        {
            if (mDouble < 1.0)
            {
                phosphor::logging::log<phosphor::logging::level::DEBUG>(
                    "Could not find mValue and B value with enough "
                    "precision.");
                return false;
            }
            break;
        }
        // can't multiply M any more, max precision reached
        else if (mDouble * 10 > maxInt10)
        {
            break;
        }
        mDouble *= 10;
        rExp--;
    }

    bDouble /= std::pow(10, rExp);
    bExp = 0;

    // B too big for 10 bit variable
    while (bDouble > maxInt10 || bDouble < minInt10)
    {
        if (bExp >= maxInt4)
        {
            phosphor::logging::log<phosphor::logging::level::DEBUG>(
                "bExp Too Big, Max and Min range need to be adjusted");
            return false;
        }
        bDouble /= 10;
        bExp++;
    }

    while (((fabs(bDouble) - floor(fabs(bDouble))) / fabs(bDouble)) >
           (1.0 / 255))
    {
        if (bExp <= minInt4)
        {
            phosphor::logging::log<phosphor::logging::level::DEBUG>(
                "bExp Too Small, Max and Min range need to be adjusted");
            return false;
        }
        bDouble *= 10;
        bExp -= 1;
    }

    mValue = static_cast<int16_t>(std::round(mDouble)) & maxInt10;
    bValue = static_cast<int16_t>(bDouble) & maxInt10;

    return true;
}

static inline uint8_t
    scaleIPMIValueFromDouble(const double value, const uint16_t mValue,
                             const int8_t rExp, const uint16_t bValue,
                             const int8_t bExp, const bool bSigned)
{
    int32_t scaledValue =
        (value - (bValue * std::pow(10, bExp) * std::pow(10, rExp))) /
        (mValue * std::pow(10, rExp));

    if (bSigned)
    {
        if (scaledValue > std::numeric_limits<int8_t>::max() ||
            scaledValue < std::numeric_limits<int8_t>::lowest())
        {
            throw std::out_of_range("Value out of range");
        }
        return static_cast<int8_t>(scaledValue);
    }
    else
    {
        if (scaledValue > std::numeric_limits<uint8_t>::max() ||
            scaledValue < std::numeric_limits<uint8_t>::lowest())
        {
            throw std::out_of_range("Value out of range");
        }
        return static_cast<uint8_t>(scaledValue);
    }
}

static inline uint8_t getScaledIPMIValue(const double value, const double max,
                                         const double min)
{
    int16_t mValue = 0;
    int8_t rExp = 0;
    int16_t bValue = 0;
    int8_t bExp = 0;
    bool bSigned = 0;
    bool result = 0;

    result = getSensorAttributes(max, min, mValue, rExp, bValue, bExp, bSigned);
    if (!result)
    {
        throw std::runtime_error("Illegal sensor attributes");
    }
    return scaleIPMIValueFromDouble(value, mValue, rExp, bValue, bExp, bSigned);
}

} // namespace ipmi