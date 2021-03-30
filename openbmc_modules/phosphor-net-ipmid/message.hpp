#pragma once

#include <cstddef>
#include <memory>
#include <numeric>
#include <vector>

namespace message
{

enum class PayloadType : uint8_t
{
    IPMI = 0x00,
    SOL = 0x01,
    OPEN_SESSION_REQUEST = 0x10,
    OPEN_SESSION_RESPONSE = 0x11,
    RAKP1 = 0x12,
    RAKP2 = 0x13,
    RAKP3 = 0x14,
    RAKP4 = 0x15,
    INVALID = 0xFF,
};

namespace LAN
{

constexpr uint8_t requesterBMCAddress = 0x20;
constexpr uint8_t responderBMCAddress = 0x81;

namespace header
{

/**
 * @struct IPMI LAN Message Request Header
 */
struct Request
{
    uint8_t rsaddr;
    uint8_t netfn;
    uint8_t cs;
    uint8_t rqaddr;
    uint8_t rqseq;
    uint8_t cmd;
} __attribute__((packed));

/**
 * @struct IPMI LAN Message Response Header
 */
struct Response
{
    uint8_t rqaddr;
    uint8_t netfn;
    uint8_t cs;
    uint8_t rsaddr;
    uint8_t rqseq;
    uint8_t cmd;
} __attribute__((packed));

} // namespace header

namespace trailer
{

/**
 * @struct IPMI LAN Message Trailer
 */
struct Request
{
    uint8_t checksum;
} __attribute__((packed));

using Response = Request;

} // namespace trailer

} // namespace LAN

/**
 * @brief Calculate 8 bit 2's complement checksum
 *
 * Initialize checksum to 0. For each byte, checksum = (checksum + byte)
 * modulo 256. Then checksum = - checksum. When the checksum and the
 * bytes are added together, modulo 256, the result should be 0.
 */
static inline uint8_t crc8bit(const uint8_t* ptr, const size_t len)
{
    return (0x100 - std::accumulate(ptr, ptr + len, 0));
}

/**
 * @struct Message
 *
 * IPMI message is data encapsulated in an IPMI Session packet. The IPMI
 * Session packets are encapsulated in RMCP packets, which are encapsulated in
 * UDP datagrams. Refer Section 13.5 of IPMI specification(IPMI Messages
 * Encapsulation Under RMCP). IPMI payload is a special class of data
 * encapsulated in an IPMI session packet.
 */
struct Message
{
    static constexpr uint32_t MESSAGE_INVALID_SESSION_ID = 0xBADBADFF;

    Message() :
        payloadType(PayloadType::INVALID),
        rcSessionID(Message::MESSAGE_INVALID_SESSION_ID),
        bmcSessionID(Message::MESSAGE_INVALID_SESSION_ID)
    {
    }

    /**
     * @brief Special behavior for copy constructor
     *
     * Based on incoming message state, the resulting message will have a
     * pre-baked state. This is used to simplify the flows for creating a
     * response message. For each pre-session state, the response message is
     * actually a different type of message. Once the session has been
     * established, the response type is the same as the request type.
     */
    Message(const Message& other) :
        isPacketEncrypted(other.isPacketEncrypted),
        isPacketAuthenticated(other.isPacketAuthenticated),
        payloadType(other.payloadType), rcSessionID(other.rcSessionID),
        bmcSessionID(other.bmcSessionID)
    {
        // special behavior for rmcp+ session creation
        if (PayloadType::OPEN_SESSION_REQUEST == other.payloadType)
        {
            payloadType = PayloadType::OPEN_SESSION_RESPONSE;
        }
        else if (PayloadType::RAKP1 == other.payloadType)
        {
            payloadType = PayloadType::RAKP2;
        }
        else if (PayloadType::RAKP3 == other.payloadType)
        {
            payloadType = PayloadType::RAKP4;
        }
    }
    Message& operator=(const Message&) = default;
    Message(Message&&) = default;
    Message& operator=(Message&&) = default;
    ~Message() = default;

    /**
     * @brief Extract the command from the IPMI payload
     *
     * @return Command ID in the incoming message
     */
    uint32_t getCommand()
    {
        uint32_t command = 0;

        command |= (static_cast<uint8_t>(payloadType) << 16);
        if (payloadType == PayloadType::IPMI)
        {
            auto request =
                reinterpret_cast<LAN::header::Request*>(payload.data());
            command |= request->netfn << 8;
            command |= request->cmd;
        }
        return command;
    }

    /**
     * @brief Create the response IPMI message
     *
     * The IPMI outgoing message is constructed out of payload and the
     * corresponding fields are populated. For the payload type IPMI, the
     * LAN message header and trailer are added.
     *
     * @param[in] output - Payload for outgoing message
     *
     * @return Outgoing message on success and nullptr on failure
     */
    std::shared_ptr<Message> createResponse(std::vector<uint8_t>& output)
    {
        // SOL packets don't reply; return NULL
        if (payloadType == PayloadType::SOL)
        {
            return nullptr;
        }
        auto outMessage = std::make_shared<Message>(*this);

        if (payloadType == PayloadType::IPMI)
        {
            outMessage->payloadType = PayloadType::IPMI;

            outMessage->payload.resize(sizeof(LAN::header::Response) +
                                       output.size() +
                                       sizeof(LAN::trailer::Response));

            auto reqHeader =
                reinterpret_cast<LAN::header::Request*>(payload.data());
            auto respHeader = reinterpret_cast<LAN::header::Response*>(
                outMessage->payload.data());

            // Add IPMI LAN Message Response Header
            respHeader->rqaddr = reqHeader->rqaddr;
            respHeader->netfn = reqHeader->netfn | 0x04;
            respHeader->cs = crc8bit(&(respHeader->rqaddr), 2);
            respHeader->rsaddr = reqHeader->rsaddr;
            respHeader->rqseq = reqHeader->rqseq;
            respHeader->cmd = reqHeader->cmd;

            auto assembledSize = sizeof(LAN::header::Response);

            // Copy the output by the execution of the command
            std::copy(output.begin(), output.end(),
                      outMessage->payload.begin() + assembledSize);
            assembledSize += output.size();

            // Add the IPMI LAN Message Trailer
            auto trailer = reinterpret_cast<LAN::trailer::Response*>(
                outMessage->payload.data() + assembledSize);
            trailer->checksum = crc8bit(&respHeader->rsaddr, assembledSize - 3);
        }
        else
        {
            outMessage->payload = output;
        }
        return outMessage;
    }

    bool isPacketEncrypted;     // Message's Encryption Status
    bool isPacketAuthenticated; // Message's Authentication Status
    PayloadType payloadType;    // Type of message payload (IPMI,SOL ..etc)
    uint32_t rcSessionID;       // Remote Client's Session ID
    uint32_t bmcSessionID;      // BMC's session ID
    uint32_t sessionSeqNum;     // Session Sequence Number

    /** @brief Message payload
     *
     *  “Payloads” are a capability specified for RMCP+ that enable an IPMI
     *  session to carry types of traffic that are in addition to IPMI Messages.
     *  Payloads can be ‘standard’ or ‘OEM’.Standard payload types include IPMI
     *  Messages, messages for session setup under RMCP+, and the payload for
     *  the “Serial Over LAN” capability introduced in IPMI v2.0.
     */
    std::vector<uint8_t> payload;
};

} // namespace message
