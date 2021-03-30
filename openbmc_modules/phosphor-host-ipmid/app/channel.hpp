#include "nlohmann/json.hpp"

#include <ipmid/api.hpp>

/** @brief The set channel access IPMI command.
 *
 *  @param[in] netfn
 *  @param[in] cmd
 *  @param[in] request
 *  @param[in,out] response
 *  @param[out] data_len
 *  @param[in] context
 *
 *  @return IPMI_CC_OK on success, non-zero otherwise.
 */
ipmi_ret_t ipmi_set_channel_access(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                   ipmi_request_t request,
                                   ipmi_response_t response,
                                   ipmi_data_len_t data_len,
                                   ipmi_context_t context);

/** @brief The get channel access IPMI command.
 *
 *  @param[in] netfn
 *  @param[in] cmd
 *  @param[in] request
 *  @param[in,out] response
 *  @param[out] data_len
 *  @param[in] context
 *
 *  @return IPMI_CC_OK on success, non-zero otherwise.
 */
ipmi_ret_t ipmi_get_channel_access(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                   ipmi_request_t request,
                                   ipmi_response_t response,
                                   ipmi_data_len_t data_len,
                                   ipmi_context_t context);

/** @brief The get channel info IPMI command.
 *
 *  @param[in] netfn
 *  @param[in] cmd
 *  @param[in] request
 *  @param[in,out] response
 *  @param[out] data_len
 *  @param[in] context
 *
 *  @return IPMI_CC_OK on success, non-zero otherwise.
 */
ipmi_ret_t ipmi_app_channel_info(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                 ipmi_request_t request,
                                 ipmi_response_t response,
                                 ipmi_data_len_t data_len,
                                 ipmi_context_t context);

/** @brief this command is used to look up what authentication, integrity,
 *  confidentiality algorithms are supported.
 *
 *  @ param ctx - context pointer
 *  @ param channelNumber - channel number
 *  @ param payloadType - payload type
 *  @ param listIndex - list index
 *  @ param algoSelectBit - list algorithms
 *
 *  @returns ipmi completion code plus response data
 *  - rspChannel - channel number for authentication algorithm.
 *  - rspRecords - cipher suite records.
 **/
ipmi::RspType<uint8_t,             // Channel Number
              std::vector<uint8_t> // Cipher Records
              >
    getChannelCipherSuites(ipmi::Context::ptr ctx, uint4_t channelNumber,
                           uint4_t reserved1, uint8_t payloadType,
                           uint6_t listIndex, uint1_t reserved2,
                           uint1_t algoSelectBit);

namespace cipher
{

static constexpr auto listCipherSuite = 0x80;

using Json = nlohmann::json;
static constexpr auto configFile = "/usr/share/ipmi-providers/cipher_list.json";
static constexpr auto cipher = "cipher";
static constexpr auto stdCipherSuite = 0xC0;
static constexpr auto oemCipherSuite = 0xC1;
static constexpr auto oem = "oemiana";
static constexpr auto auth = "authentication";
static constexpr auto integrity = "integrity";
static constexpr auto integrityTag = 0x40;
static constexpr auto conf = "confidentiality";
static constexpr auto confTag = 0x80;

} // namespace cipher
