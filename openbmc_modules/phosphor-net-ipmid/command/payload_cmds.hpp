#pragma once

#include "message_handler.hpp"

#include <vector>

namespace sol
{

namespace command
{

constexpr uint8_t IPMI_CC_PAYLOAD_ALREADY_ACTIVE = 0x80;
constexpr uint8_t IPMI_CC_PAYLOAD_TYPE_DISABLED = 0x81;
constexpr uint8_t IPMI_CC_PAYLOAD_ACTIVATION_LIMIT = 0x82;
constexpr uint8_t IPMI_CC_PAYLOAD_WITH_ENCRYPTION = 0x83;
constexpr uint8_t IPMI_CC_PAYLOAD_WITHOUT_ENCRYPTION = 0x84;

/** @struct ActivatePayloadRequest
 *
 *  IPMI payload for Activate Payload command request.
 */
struct ActivatePayloadRequest
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t payloadType : 6; //!< Payload type.
    uint8_t reserved1 : 2;   //!< Reserved.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved1 : 2;   //!< Payload type.
    uint8_t payloadType : 6; //!< Payload type.
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t payloadInstance : 4; //!< Payload instance.
    uint8_t reserved2 : 4;       //!< Reserved.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved2 : 4;       //!< Reserved.
    uint8_t payloadInstance : 4; //!< Payload instance.
#endif

    /** @brief The following Auxiliary Request Data applies only for payload
     *         SOL only.
     */
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t reserved4 : 1;  //!< Reserved.
    uint8_t handshake : 1;  //!< SOL startup handshake.
    uint8_t alert : 2;      //!< Shared serial alert behavior.
    uint8_t reserved3 : 1;  //!< Reserved.
    uint8_t testMode : 1;   //!< Test mode.
    uint8_t auth : 1;       //!< If true, activate payload with authentication.
    uint8_t encryption : 1; //!< If true, activate payload with encryption.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t encryption : 1; //!< If true, activate payload with encryption.
    uint8_t auth : 1;       //!< If true, activate payload with authentication.
    uint8_t testMode : 1;   //!< Test mode.
    uint8_t reserved3 : 1;  //!< Reserved.
    uint8_t alert : 2;      //!< Shared serial alert behavior.
    uint8_t handshake : 1;  //!< SOL startup handshake.
    uint8_t reserved4 : 1;  //!< Reserved.
#endif

    uint8_t reserved5; //!< Reserved.
    uint8_t reserved6; //!< Reserved.
    uint8_t reserved7; //!< Reserved.
} __attribute__((packed));

/** @struct ActivatePayloadResponse
 *
 *  IPMI payload for Activate Payload command response.
 */
struct ActivatePayloadResponse
{
    uint8_t completionCode; //!< Completion code.
    uint8_t reserved1;      //!< Reserved.
    uint8_t reserved2;      //!< Reserved.
    uint8_t reserved3;      //!< Reserved.

    // Test Mode
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t testMode : 1;  //!< Test mode.
    uint8_t reserved4 : 7; //!< Reserved.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved4 : 7; //!< Reserved.
    uint8_t testMode : 1;  //!< Test mode.
#endif

    uint16_t inPayloadSize;  //!< Inbound payload size
    uint16_t outPayloadSize; //!< Outbound payload size.
    uint16_t portNum;        //!< Payload UDP port number.
    uint16_t vlanNum;        //!< Payload VLAN number.
} __attribute__((packed));

/** @brief Activate Payload Command.
 *
 *  This command is used for activating and deactivating a payload type under a
 *  given IPMI session. The UDP Port number for SOL is the same as the port that
 *  was used to establish the IPMI session.
 *
 *  @param[in] inPayload - Request data for the command.
 *  @param[in] handler - Reference to the message handler.
 *
 *  @return Response data for the command
 */
std::vector<uint8_t> activatePayload(const std::vector<uint8_t>& inPayload,
                                     const message::Handler& handler);

constexpr uint8_t IPMI_CC_PAYLOAD_DEACTIVATED = 0x80;

/** @struct DeactivatePayloadRequest
 *
 *  IPMI payload for Deactivate Payload command request.
 */
struct DeactivatePayloadRequest
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t payloadType : 6; //!< Payload type.
    uint8_t reserved1 : 2;   //!< Reserved.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved1 : 2;   //!< Payload type.
    uint8_t payloadType : 6; //!< Reserved.
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t payloadInstance : 4; //!< Payload instance.
    uint8_t reserved2 : 4;       //!< Reserved.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved2 : 4;       //!< Reserved.
    uint8_t payloadInstance : 4; //!< Payload instance.
#endif

    /** @brief No auxiliary data for payload type SOL */
    uint8_t auxData1; //!< Auxiliary data 1
    uint8_t auxData2; //!< Auxiliary data 2
    uint8_t auxData3; //!< Auxiliary data 3
    uint8_t auxData4; //!< Auxiliary data 4
} __attribute__((packed));

/** @struct DeactivatePayloadResponse
 *
 * IPMI payload for Deactivate Payload Command response.
 */
struct DeactivatePayloadResponse
{
    uint8_t completionCode; //!< Completion code
} __attribute__((packed));

/** @brief Deactivate Payload Command.
 *
 *  This command is used to terminate use of a given payload on an IPMI session.
 *  This type of traffic then becomes freed for activation by another session,
 *  or for possible re-activation under the present session.The Deactivate
 *  Payload command does not cause the session to be terminated. The Close
 *  Session command should be used for that purpose. A remote console
 *  terminating a application does not need to explicitly deactivate payload(s)
 *  prior to session. When a session terminates all payloads that were active
 *  under that session are automatically deactivated by the BMC.
 *
 * @param[in] inPayload - Request data for the command.
 * @param[in] handler - Reference to the message handler.
 *
 * @return Response data for the command.
 */
std::vector<uint8_t> deactivatePayload(const std::vector<uint8_t>& inPayload,
                                       const message::Handler& handler);

/** @struct GetPayloadStatusRequest
 *
 *  IPMI payload for Get Payload Activation Status command request.
 */
struct GetPayloadStatusRequest
{
    uint8_t payloadType; //!< Payload type
} __attribute__((packed));

/** @struct GetPayloadStatusResponse
 *
 *  IPMI payload for Get Payload Activation Status command response.
 */
struct GetPayloadStatusResponse
{
    uint8_t completionCode; //!< Completion code.

    uint8_t capacity; //!< Instance capacity.

    /* @brief Activation Status. */
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t instance1 : 1; //!< If true, Instance 1 is activated.
    uint8_t instance2 : 1; //!< If true, Instance 2 is activated.
    uint8_t instance3 : 1; //!< If true, Instance 3 is activated.
    uint8_t instance4 : 1; //!< If true, Instance 4 is activated.
    uint8_t instance5 : 1; //!< If true, Instance 5 is activated.
    uint8_t instance6 : 1; //!< If true, Instance 6 is activated.
    uint8_t instance7 : 1; //!< If true, Instance 7 is activated.
    uint8_t instance8 : 1; //!< If true, Instance 8 is activated.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t instance8 : 1; //!< If true, Instance 8 is activated.
    uint8_t instance7 : 1; //!< If true, Instance 7 is activated.
    uint8_t instance6 : 1; //!< If true, Instance 6 is activated.
    uint8_t instance5 : 1; //!< If true, Instance 5 is activated.
    uint8_t instance4 : 1; //!< If true, Instance 4 is activated.
    uint8_t instance3 : 1; //!< If true, Instance 3 is activated.
    uint8_t instance2 : 1; //!< If true, Instance 2 is activated.
    uint8_t instance1 : 1; //!< If true, Instance 1 is activated.
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t instance9 : 1;  //!< If true, Instance 9 is activated.
    uint8_t instance10 : 1; //!< If true, Instance 10 is activated.
    uint8_t instance11 : 1; //!< If true, Instance 11 is activated.
    uint8_t instance12 : 1; //!< If true, Instance 12 is activated.
    uint8_t instance13 : 1; //!< If true, Instance 13 is activated.
    uint8_t instance14 : 1; //!< If true, Instance 14 is activated.
    uint8_t instance15 : 1; //!< If true, Instance 15 is activated.
    uint8_t instance16 : 1; //!< If true, Instance 16 is activated.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t instance16 : 1; //!< If true, Instance 16 is activated.
    uint8_t instance15 : 1; //!< If true, Instance 15 is activated.
    uint8_t instance14 : 1; //!< If true, Instance 14 is activated.
    uint8_t instance13 : 1; //!< If true, Instance 13 is activated.
    uint8_t instance12 : 1; //!< If true, Instance 12 is activated.
    uint8_t instance11 : 1; //!< If true, Instance 11 is activated.
    uint8_t instance10 : 1; //!< If true, Instance 10 is activated.
    uint8_t instance9 : 1;  //!< If true, Instance 9 is activated.
#endif
} __attribute__((packed));

/** @brief Get Payload Activation Status Command.
 *
 *  This command returns how many instances of a given payload type are
 *  presently activated, and how many total instances can be activated.
 *
 *  @param[in] inPayload - Request Data for the command.
 *  @param[in] handler - Reference to the Message Handler.
 *
 *  @return Response data for the command
 */
std::vector<uint8_t> getPayloadStatus(const std::vector<uint8_t>& inPayload,
                                      const message::Handler& handler);

/** @struct GetPayloadInfoRequest
 *
 *  IPMI payload for Get Payload Instance info command request.
 */
struct GetPayloadInfoRequest
{
    uint8_t payloadType;     //!< Payload type
    uint8_t payloadInstance; //!< Payload instance
} __attribute__((packed));

/** @struct GetPayloadInfoResponse
 *
 *  IPMI payload for Get Payload Instance info command response.
 */
struct GetPayloadInfoResponse
{
    uint8_t completionCode; //!< Completion code.
    uint32_t sessionID;     //!< Session ID
    uint8_t portNumber;     //!< Port number
    uint8_t reserved[7];    //!< Reserved
} __attribute__((packed));

/** @brief Get Payload Instance Info Command.
 *
 *  This command returns information about a specific instance of a payload
 *  type. Session ID is returned by this command
 *
 *  @param[in] inPayload - Request Data for the command.
 *  @param[in] handler - Reference to the Message Handler.
 *
 *  @return Response data for the command
 */
std::vector<uint8_t> getPayloadInfo(const std::vector<uint8_t>& inPayload,
                                    const message::Handler& handler);

} // namespace command

} // namespace sol
