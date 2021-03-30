#pragma once
#include "stream.hpp"

#include <chrono>

namespace openpower
{
namespace pels
{

/**
 * @brief A structure that contains a PEL timestamp in BCD.
 */
struct BCDTime
{
    uint8_t yearMSB;
    uint8_t yearLSB;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t hundredths;

    BCDTime() :
        yearMSB(0), yearLSB(0), month(0), day(0), hour(0), minutes(0),
        seconds(0), hundredths(0)
    {
    }

    BCDTime(uint8_t yearMSB, uint8_t yearLSB, uint8_t month, uint8_t day,
            uint8_t hour, uint8_t minutes, uint8_t seconds,
            uint8_t hundredths) :
        yearMSB(yearMSB),
        yearLSB(yearLSB), month(month), day(day), hour(hour), minutes(minutes),
        seconds(seconds), hundredths(hundredths)
    {
    }

    bool operator==(const BCDTime& right) const;
    bool operator!=(const BCDTime& right) const;

} __attribute__((packed));

/**
 * @brief Converts a time_point into a BCD time
 *
 * @param[in] time - the time_point to convert
 * @return BCDTime - the BCD time
 */
BCDTime getBCDTime(std::chrono::time_point<std::chrono::system_clock>& time);

/**
 * @brief Converts the number of milliseconds since the epoch into BCD time
 *
 * @param[in] milliseconds - Number of milliseconds since the epoch
 * @return BCDTime - the BCD time
 */
BCDTime getBCDTime(uint64_t milliseconds);

/**
 * @brief Converts a number to a BCD.
 *
 * For example 32 -> 0x32.
 *
 * Source: PLDM repository
 *
 * @param[in] value - the value to convert.
 *
 * @return T - the BCD value
 */
template <typename T>
T toBCD(T decimal)
{
    T bcd = 0;
    T remainder = 0;
    auto count = 0;

    while (decimal)
    {
        remainder = decimal % 10;
        bcd = bcd + (remainder << count);
        decimal = decimal / 10;
        count += 4;
    }

    return bcd;
}

/**
 * @brief Stream extraction operator for BCDTime
 *
 * @param[in] s - the Stream
 * @param[out] time - the BCD time
 *
 * @return Stream&
 */
Stream& operator>>(Stream& s, BCDTime& time);

/**
 * @brief Stream insertion operator for BCDTime
 *
 * @param[in/out] s - the Stream
 * @param[in] time - the BCD time
 *
 * @return Stream&
 */
Stream& operator<<(Stream& s, const BCDTime& time);

} // namespace pels
} // namespace openpower
