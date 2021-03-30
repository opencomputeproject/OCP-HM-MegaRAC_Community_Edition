#pragma once

#include "handler.hpp"

#include <ipmid/api.h>

#include <cstdint>
#include <string>

namespace ethstats
{

/**
 * @brief Ethstat Request structure.
 */
struct EthStatRequest
{
    std::uint8_t statId;
    std::uint8_t if_name_len;
} __attribute__((packed));

/**
 * @brief Ethstat Reply structure.
 */
struct EthStatReply
{
    std::uint8_t statId;
    std::uint64_t value;
} __attribute__((packed));

enum EthernetStatisticsIds
{
    RX_BYTES = 0,
    RX_COMPRESSED = 1,
    RX_CRC_ERRORS = 2,
    RX_DROPPED = 3,
    RX_ERRORS = 4,
    RX_FIFO_ERRORS = 5,
    RX_FRAME_ERRORS = 6,
    RX_LENGTH_ERRORS = 7,
    RX_MISSED_ERRORS = 8,
    RX_NOHANDLER = 9,
    RX_OVER_ERRORS = 10,
    RX_PACKETS = 11,
    TX_ABORTED_ERRORS = 12,
    TX_BYTES = 13,
    TX_CARRIER_ERRORS = 14,
    TX_COMPRESSED = 15,
    TX_DROPPED = 16,
    TX_ERRORS = 17,
    TX_FIFO_ERRORS = 18,
    TX_HEARTBEAT_ERRORS = 19,
    TX_PACKETS = 20,
    TX_WINDOW_ERRORS = 21,
};

/**
 * Handle the OEM IPMI EthStat Command.
 *
 * @param[in] reqBuf - the IPMI request buffer.
 * @param[in,out] replyCmdBuf - the IPMI reply buffer.
 * @param[in,out] dataLen - the length of the request and reply.
 * @param[in] handler - pointer to ethstats implementation.
 * @return the IPMI result code.
 */
ipmi_ret_t handleEthStatCommand(const std::uint8_t* reqBuf,
                                std::uint8_t* replyCmdBuf, size_t* dataLen,
                                const EthStatsInterface* handler);

/**
 * Given an ethernet if_name and a field, build the full path.
 *
 * @param[in] ifName - the ethernet interface's name.
 * @param[in] field - the name of the statistic
 * @return the full path of the file to read for the statistic for that
 * interface name.
 */
std::string buildPath(const std::string& ifName, const std::string& field);

} // namespace ethstats
