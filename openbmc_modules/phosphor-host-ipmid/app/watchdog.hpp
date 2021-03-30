#pragma once

#include <ipmid/api.hpp>

/** @brief The RESET watchdog IPMI command.
 */
ipmi::RspType<> ipmiAppResetWatchdogTimer();

/**@brief The setWatchdogTimer ipmi command.
 *
 * @param
 * - timerUse
 * - dontStopTimer
 * - dontLog
 * - timerAction
 * - pretimeout
 * - expireFlags
 * - initialCountdown
 *
 * @return completion code on success.
 **/
ipmi::RspType<> ipmiSetWatchdogTimer(
    uint3_t timerUse, uint3_t reserved, bool dontStopTimer, bool dontLog,
    uint3_t timeoutAction, uint1_t reserved1, uint3_t preTimeoutInterrupt,
    uint1_t reserved2, uint8_t preTimeoutInterval, std::bitset<8> expFlagValue,
    uint16_t initialCountdown);

/**@brief The getWatchdogTimer ipmi command.
 *
 * @return
 * - timerUse
 * - timerActions
 * - pretimeout
 * - timeruseFlags
 * - initialCountdown
 * - presentCountdown
 **/
ipmi::RspType<uint3_t, uint3_t, bool, bool,       // timerUse
              uint3_t, uint1_t, uint3_t, uint1_t, // timerAction
              uint8_t,                            // pretimeout
              std::bitset<8>,                     // expireFlags
              uint16_t, // initial Countdown - Little Endian (deciseconds)
              uint16_t  // present Countdown - Little Endian (deciseconds)
              >
    ipmiGetWatchdogTimer();
