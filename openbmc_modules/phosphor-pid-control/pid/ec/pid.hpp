#pragma once

#include <cstdint>

namespace ec
{

typedef struct
{
    double min;
    double max;
} limits_t;

/* Note: If you update these structs you need to update the copy code in
 * pid/util.cpp.
 */
typedef struct
{
    bool initialized; // has pid been initialized

    double ts;         // sample time in seconds
    double integral;   // intergal of error
    double lastOutput; // value of last output

    double proportionalCoeff; // coeff for P
    double integralCoeff;     // coeff for I
    double feedFwdOffset;     // offset coeff for feed-forward term
    double feedFwdGain;       // gain for feed-forward term

    limits_t integralLimit; // clamp of integral
    limits_t outLim;        // clamp of output
    double slewNeg;
    double slewPos;
    double positiveHysteresis;
    double negativeHysteresis;
} pid_info_t;

double pid(pid_info_t* pidinfoptr, double input, double setpoint);

/* Condensed version for use by the configuration. */
struct pidinfo
{
    double ts;                  // sample time in seconds
    double proportionalCoeff;   // coeff for P
    double integralCoeff;       // coeff for I
    double feedFwdOffset;       // offset coeff for feed-forward term
    double feedFwdGain;         // gain for feed-forward term
    ec::limits_t integralLimit; // clamp of integral
    ec::limits_t outLim;        // clamp of output
    double slewNeg;
    double slewPos;
    double positiveHysteresis;
    double negativeHysteresis;
};

} // namespace ec
