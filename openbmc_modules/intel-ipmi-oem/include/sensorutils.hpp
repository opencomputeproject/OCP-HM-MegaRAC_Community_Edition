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
#include <algorithm>
#include <cmath>
#include <iostream>

namespace ipmi
{
static constexpr int16_t maxInt10 = 0x1FF;
static constexpr int16_t minInt10 = -0x200;
static constexpr int8_t maxInt4 = 7;
static constexpr int8_t minInt4 = -8;

// Helper function to avoid repeated complicated expression
// TODO(): Refactor to add a proper sensorutils.cpp file,
// instead of putting everything in this header as it is now,
// so that helper functions can be correctly hidden from callers.
static inline bool baseInRange(double base)
{
    auto min10 = static_cast<double>(minInt10);
    auto max10 = static_cast<double>(maxInt10);

    return ((base >= min10) && (base <= max10));
}

// Helper function for internal use by getSensorAttributes()
// Ensures floating-point "base" is within bounds,
// and adjusts integer exponent "expShift" accordingly.
// To minimize data loss when later truncating to integer,
// the floating-point "base" will be as large as possible,
// but still within the bounds (minInt10,maxInt10).
// The bounds of "expShift" are (minInt4,maxInt4).
// Consider this equation: n = base * (10.0 ** expShift)
// This function will try to maximize "base",
// adjusting "expShift" to keep the value "n" unchanged,
// while keeping base and expShift within bounds.
// Returns true if successful, modifies values in-place
static inline bool scaleFloatExp(double& base, int8_t& expShift)
{
    // Comparing with zero should be OK, zero is special in floating-point
    // If base is exactly zero, no adjustment of the exponent is necessary
    if (base == 0.0)
    {
        return true;
    }

    // As long as base value is within allowed range, expand precision
    // This will help to avoid loss when later rounding to integer
    while (baseInRange(base))
    {
        if (expShift <= minInt4)
        {
            // Already at the minimum expShift, can not decrement it more
            break;
        }

        // Multiply by 10, but shift decimal point to the left, no net change
        base *= 10.0;
        --expShift;
    }

    // As long as base value is *not* within range, shrink precision
    // This will pull base value closer to zero, thus within range
    while (!(baseInRange(base)))
    {
        if (expShift >= maxInt4)
        {
            // Already at the maximum expShift, can not increment it more
            break;
        }

        // Divide by 10, but shift decimal point to the right, no net change
        base /= 10.0;
        ++expShift;
    }

    // If the above loop was not able to pull it back within range,
    // the base value is beyond what expShift can represent, return false.
    return baseInRange(base);
}

// Helper function for internal use by getSensorAttributes()
// Ensures integer "ibase" is no larger than necessary,
// by normalizing it so that the decimal point shift is in the exponent,
// whenever possible.
// This provides more consistent results,
// as many equivalent solutions are collapsed into one consistent solution.
// If integer "ibase" is a clean multiple of 10,
// divide it by 10 (this is lossless), so it is closer to zero.
// Also modify floating-point "dbase" at the same time,
// as both integer and floating-point base share the same expShift.
// Example: (ibase=300, expShift=2) becomes (ibase=3, expShift=4)
// because the underlying value is the same: 200*(10**2) == 2*(10**4)
// Always successful, modifies values in-place
static inline void normalizeIntExp(int16_t& ibase, int8_t& expShift,
                                   double& dbase)
{
    for (;;)
    {
        // If zero, already normalized, ensure exponent also zero
        if (ibase == 0)
        {
            expShift = 0;
            break;
        }

        // If not cleanly divisible by 10, already normalized
        if ((ibase % 10) != 0)
        {
            break;
        }

        // If exponent already at max, already normalized
        if (expShift >= maxInt4)
        {
            break;
        }

        // Bring values closer to zero, correspondingly shift exponent,
        // without changing the underlying number that this all represents,
        // similar to what is done by scaleFloatExp().
        // The floating-point base must be kept in sync with the integer base,
        // as both floating-point and integer share the same exponent.
        ibase /= 10;
        dbase /= 10.0;
        ++expShift;
    }
}

// The IPMI equation:
// y = (Mx + (B * 10^(bExp))) * 10^(rExp)
// Section 36.3 of this document:
// https://www.intel.com/content/dam/www/public/us/en/documents/product-briefs/ipmi-second-gen-interface-spec-v2-rev1-1.pdf
//
// The goal is to exactly match the math done by the ipmitool command,
// at the other side of the interface:
// https://github.com/ipmitool/ipmitool/blob/42a023ff0726c80e8cc7d30315b987fe568a981d/lib/ipmi_sdr.c#L360
//
// To use with Wolfram Alpha, make all variables single letters
// bExp becomes E, rExp becomes R
// https://www.wolframalpha.com/input/?i=y%3D%28%28M*x%29%2B%28B*%2810%5EE%29%29%29*%2810%5ER%29
static inline bool getSensorAttributes(const double max, const double min,
                                       int16_t& mValue, int8_t& rExp,
                                       int16_t& bValue, int8_t& bExp,
                                       bool& bSigned)
{
    if (!(std::isfinite(min)))
    {
        std::cerr << "getSensorAttributes: Min value is unusable\n";
        return false;
    }
    if (!(std::isfinite(max)))
    {
        std::cerr << "getSensorAttributes: Max value is unusable\n";
        return false;
    }

    // Because NAN has already been tested for, this comparison works
    if (max <= min)
    {
        std::cerr << "getSensorAttributes: Max must be greater than min\n";
        return false;
    }

    // Given min and max, we must solve for M, B, bExp, rExp
    // y comes in from D-Bus (the actual sensor reading)
    // x is calculated from y by scaleIPMIValueFromDouble() below
    // If y is min, x should equal = 0 (or -128 if signed)
    // If y is max, x should equal 255 (or 127 if signed)
    double fullRange = max - min;
    double lowestX;

    rExp = 0;
    bExp = 0;

    // TODO(): The IPMI document is ambiguous, as to whether
    // the resulting byte should be signed or unsigned,
    // essentially leaving it up to the caller.
    // The document just refers to it as "raw reading",
    // or "byte of reading", without giving further details.
    // Previous code set it signed if min was less than zero,
    // so I'm sticking with that, until I learn otherwise.
    if (min < 0.0)
    {
        // TODO(): It would be worth experimenting with the range (-127,127),
        // instead of the range (-128,127), because this
        // would give good symmetry around zero, and make results look better.
        // Divide by 254 instead of 255, and change -128 to -127 elsewhere.
        bSigned = true;
        lowestX = -128.0;
    }
    else
    {
        bSigned = false;
        lowestX = 0.0;
    }

    // Step 1: Set y to (max - min), set x to 255, set B to 0, solve for M
    // This works, regardless of signed or unsigned,
    // because total range is the same.
    double dM = fullRange / 255.0;

    // Step 2: Constrain M, and set rExp accordingly
    if (!(scaleFloatExp(dM, rExp)))
    {
        std::cerr << "getSensorAttributes: Multiplier range exceeds scale (M="
                  << dM << ", rExp=" << (int)rExp << ")\n";
        return false;
    }

    mValue = static_cast<int16_t>(std::round(dM));

    normalizeIntExp(mValue, rExp, dM);

    // The multiplier can not be zero, for obvious reasons
    if (mValue == 0)
    {
        std::cerr << "getSensorAttributes: Multiplier range below scale\n";
        return false;
    }

    // Step 3: set y to min, set x to min, keep M and rExp, solve for B
    // If negative, x will be -128 (the most negative possible byte), not 0

    // Solve the IPMI equation for B, instead of y
    // https://www.wolframalpha.com/input/?i=solve+y%3D%28%28M*x%29%2B%28B*%2810%5EE%29%29%29*%2810%5ER%29+for+B
    // B = 10^(-rExp - bExp) (y - M 10^rExp x)
    // TODO(): Compare with this alternative solution from SageMathCell
    // https://sagecell.sagemath.org/?z=eJyrtC1LLNJQr1TX5KqAMCuATF8I0xfIdIIwnYDMIteKAggPxAIKJMEFkiACxfk5Zaka0ZUKtrYKGhq-CloKFZoK2goaTkCWhqGBgpaWAkilpqYmQgBklmasjoKTJgDAECTH&lang=sage&interacts=eJyLjgUAARUAuQ==
    double dB = std::pow(10.0, ((-rExp) - bExp)) *
                (min - ((dM * std::pow(10.0, rExp) * lowestX)));

    // Step 4: Constrain B, and set bExp accordingly
    if (!(scaleFloatExp(dB, bExp)))
    {
        std::cerr << "getSensorAttributes: Offset (B=" << dB
                  << ", bExp=" << (int)bExp
                  << ") exceeds multiplier scale (M=" << dM
                  << ", rExp=" << (int)rExp << ")\n";
        return false;
    }

    bValue = static_cast<int16_t>(std::round(dB));

    normalizeIntExp(bValue, bExp, dB);

    // Unlike the multiplier, it is perfectly OK for bValue to be zero
    return true;
}

static inline uint8_t
    scaleIPMIValueFromDouble(const double value, const int16_t mValue,
                             const int8_t rExp, const int16_t bValue,
                             const int8_t bExp, const bool bSigned)
{
    // Avoid division by zero below
    if (mValue == 0)
    {
        throw std::out_of_range("Scaling multiplier is uninitialized");
    }

    auto dM = static_cast<double>(mValue);
    auto dB = static_cast<double>(bValue);

    // Solve the IPMI equation for x, instead of y
    // https://www.wolframalpha.com/input/?i=solve+y%3D%28%28M*x%29%2B%28B*%2810%5EE%29%29%29*%2810%5ER%29+for+x
    // x = (10^(-rExp) (y - B 10^(rExp + bExp)))/M and M 10^rExp!=0
    // TODO(): Compare with this alternative solution from SageMathCell
    // https://sagecell.sagemath.org/?z=eJyrtC1LLNJQr1TX5KqAMCuATF8I0xfIdIIwnYDMIteKAggPxAIKJMEFkiACxfk5Zaka0ZUKtrYKGhq-CloKFZoK2goaTkCWhqGBgpaWAkilpqYmQgBklmasDlAlAMB8JP0=&lang=sage&interacts=eJyLjgUAARUAuQ==
    double dX =
        (std::pow(10.0, -rExp) * (value - (dB * std::pow(10.0, rExp + bExp)))) /
        dM;

    auto scaledValue = static_cast<int32_t>(std::round(dX));

    int32_t minClamp;
    int32_t maxClamp;

    // Because of rounding and integer truncation of scaling factors,
    // sometimes the resulting byte is slightly out of range.
    // Still allow this, but clamp the values to range.
    if (bSigned)
    {
        minClamp = std::numeric_limits<int8_t>::lowest();
        maxClamp = std::numeric_limits<int8_t>::max();
    }
    else
    {
        minClamp = std::numeric_limits<uint8_t>::lowest();
        maxClamp = std::numeric_limits<uint8_t>::max();
    }

    auto clampedValue = std::clamp(scaledValue, minClamp, maxClamp);

    // This works for both signed and unsigned,
    // because it is the same underlying byte storage.
    return static_cast<uint8_t>(clampedValue);
}

static inline uint8_t getScaledIPMIValue(const double value, const double max,
                                         const double min)
{
    int16_t mValue = 0;
    int8_t rExp = 0;
    int16_t bValue = 0;
    int8_t bExp = 0;
    bool bSigned = false;

    bool result =
        getSensorAttributes(max, min, mValue, rExp, bValue, bExp, bSigned);
    if (!result)
    {
        throw std::runtime_error("Illegal sensor attributes");
    }

    return scaleIPMIValueFromDouble(value, mValue, rExp, bValue, bExp, bSigned);
}

} // namespace ipmi
