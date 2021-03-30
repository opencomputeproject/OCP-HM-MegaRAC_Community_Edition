#pragma once

#include "config.h"

#include "libpldm/bios.h"
#include "libpldm/bios_table.h"

#include "bios_config.hpp"
#include "bios_table.hpp"
#include "pldmd/handler.hpp"

#include <stdint.h>

#include <ctime>
#include <functional>
#include <map>
#include <vector>

namespace pldm
{

namespace responder
{

namespace bios
{

class Handler : public CmdHandler
{
  public:
    Handler();

    /** @brief Handler for GetDateTime
     *
     *  @param[in] request - Request message payload
     *  @return Response - PLDM Response message
     */
    Response getDateTime(const pldm_msg* request, size_t payloadLength);

    /** @brief Handler for GetBIOSTable
     *
     *  @param[in] request - Request message
     *  @param[in] payload_length - Request message payload length
     *  @return Response - PLDM Response message
     */
    Response getBIOSTable(const pldm_msg* request, size_t payloadLength);

    /** @brief Handler for GetBIOSAttributeCurrentValueByHandle
     *
     *  @param[in] request - Request message
     *  @param[in] payloadLength - Request message payload length
     *  @return Response - PLDM Response message
     */
    Response getBIOSAttributeCurrentValueByHandle(const pldm_msg* request,
                                                  size_t payloadLength);

    /** @brief Handler for SetDateTime
     *
     *  @param[in] request - Request message payload
     *  @param[in] payloadLength - Request message payload length
     *  @return Response - PLDM Response message
     */
    Response setDateTime(const pldm_msg* request, size_t payloadLength);

    /** @brief Handler for setBIOSAttributeCurrentValue
     *
     *  @param[in] request - Request message
     *  @param[in] payloadLength - Request message payload length
     *  @return Response - PLDM Response message
     */
    Response setBIOSAttributeCurrentValue(const pldm_msg* request,
                                          size_t payloadLength);

  private:
    BIOSConfig biosConfig;
};

} // namespace bios

namespace utils
{

/** @brief Convert epoch time to BCD time
 *
 *  @param[in] timeSec - Time got from epoch time in seconds
 *  @param[out] seconds - number of seconds in BCD
 *  @param[out] minutes - number of minutes in BCD
 *  @param[out] hours - number of hours in BCD
 *  @param[out] day - day of the month in BCD
 *  @param[out] month - month number in BCD
 *  @param[out] year - year number in BCD
 */
void epochToBCDTime(uint64_t timeSec, uint8_t& seconds, uint8_t& minutes,
                    uint8_t& hours, uint8_t& day, uint8_t& month,
                    uint16_t& year);

/** @brief Convert dec time to epoch time
 *
 *  @param[in] seconds - number of seconds in dec
 *  @param[in] minutes - number of minutes in dec
 *  @param[in] hours - number of hours in dec
 *  @param[in] day - day of the month in dec
 *  @param[in] month - month number in dec
 *  @param[in] year - year number in dec
 *  @return time - epoch time
 */
std::time_t timeToEpoch(uint8_t seconds, uint8_t minutes, uint8_t hours,
                        uint8_t day, uint8_t month, uint16_t year);
} // namespace utils

} // namespace responder
} // namespace pldm
