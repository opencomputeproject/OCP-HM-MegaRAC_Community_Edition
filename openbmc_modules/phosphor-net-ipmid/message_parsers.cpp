#include "message_parsers.hpp"

#include "endian.hpp"
#include "main.hpp"
#include "message.hpp"
#include "sessions_manager.hpp"

#include <memory>

namespace message
{

namespace parser
{

std::tuple<std::shared_ptr<Message>, SessionHeader>
    unflatten(std::vector<uint8_t>& inPacket)
{
    // Check if the packet has atleast the size of the RMCP Header
    if (inPacket.size() < sizeof(BasicHeader_t))
    {
        throw std::runtime_error("RMCP Header missing");
    }

    auto rmcpHeaderPtr = reinterpret_cast<BasicHeader_t*>(inPacket.data());

    // Verify if the fields in the RMCP header conforms to the specification
    if ((rmcpHeaderPtr->version != RMCP_VERSION) ||
        (rmcpHeaderPtr->rmcpSeqNum != RMCP_SEQ) ||
        (rmcpHeaderPtr->classOfMsg != RMCP_MESSAGE_CLASS_IPMI))
    {
        throw std::runtime_error("RMCP Header is invalid");
    }

    // Read the Session Header and invoke the parser corresponding to the
    // header type
    switch (static_cast<SessionHeader>(rmcpHeaderPtr->format.formatType))
    {
        case SessionHeader::IPMI15:
        {
            return std::make_tuple(ipmi15parser::unflatten(inPacket),
                                   SessionHeader::IPMI15);
        }
        case SessionHeader::IPMI20:
        {
            return std::make_tuple(ipmi20parser::unflatten(inPacket),
                                   SessionHeader::IPMI20);
        }
        default:
        {
            throw std::runtime_error("Invalid Session Header");
        }
    }
}

std::vector<uint8_t> flatten(std::shared_ptr<Message> outMessage,
                             SessionHeader authType,
                             std::shared_ptr<session::Session> session)
{
    // Call the flatten routine based on the header type
    switch (authType)
    {
        case SessionHeader::IPMI15:
        {
            return ipmi15parser::flatten(outMessage, session);
        }
        case SessionHeader::IPMI20:
        {
            return ipmi20parser::flatten(outMessage, session);
        }
        default:
        {
            return {};
        }
    }
}

} // namespace parser

namespace ipmi15parser
{

std::shared_ptr<Message> unflatten(std::vector<uint8_t>& inPacket)
{
    // Check if the packet has atleast the Session Header
    if (inPacket.size() < sizeof(SessionHeader_t))
    {
        throw std::runtime_error("IPMI1.5 Session Header Missing");
    }

    auto message = std::make_shared<Message>();

    auto header = reinterpret_cast<SessionHeader_t*>(inPacket.data());

    message->payloadType = PayloadType::IPMI;
    message->bmcSessionID = endian::from_ipmi(header->sessId);
    message->sessionSeqNum = endian::from_ipmi(header->sessSeqNum);
    message->isPacketEncrypted = false;
    message->isPacketAuthenticated = false;

    auto payloadLen = header->payloadLength;

    (message->payload)
        .assign(inPacket.data() + sizeof(SessionHeader_t),
                inPacket.data() + sizeof(SessionHeader_t) + payloadLen);

    return message;
}

std::vector<uint8_t> flatten(std::shared_ptr<Message> outMessage,
                             std::shared_ptr<session::Session> session)
{
    std::vector<uint8_t> packet(sizeof(SessionHeader_t));

    // Insert Session Header into the Packet
    auto header = reinterpret_cast<SessionHeader_t*>(packet.data());
    header->base.version = parser::RMCP_VERSION;
    header->base.reserved = 0x00;
    header->base.rmcpSeqNum = parser::RMCP_SEQ;
    header->base.classOfMsg = parser::RMCP_MESSAGE_CLASS_IPMI;
    header->base.format.formatType =
        static_cast<uint8_t>(parser::SessionHeader::IPMI15);
    header->sessSeqNum = 0;
    header->sessId = endian::to_ipmi(outMessage->rcSessionID);

    header->payloadLength = static_cast<uint8_t>(outMessage->payload.size());

    // Insert the Payload into the Packet
    packet.insert(packet.end(), outMessage->payload.begin(),
                  outMessage->payload.end());

    // Insert the Session Trailer
    packet.resize(packet.size() + sizeof(SessionTrailer_t));
    auto trailer =
        reinterpret_cast<SessionTrailer_t*>(packet.data() + packet.size());
    trailer->legacyPad = 0x00;

    return packet;
}

} // namespace ipmi15parser

namespace ipmi20parser
{

std::shared_ptr<Message> unflatten(std::vector<uint8_t>& inPacket)
{
    // Check if the packet has atleast the Session Header
    if (inPacket.size() < sizeof(SessionHeader_t))
    {
        throw std::runtime_error("IPMI2.0 Session Header Missing");
    }

    auto message = std::make_shared<Message>();

    auto header = reinterpret_cast<SessionHeader_t*>(inPacket.data());

    message->payloadType = static_cast<PayloadType>(header->payloadType & 0x3F);
    message->bmcSessionID = endian::from_ipmi(header->sessId);
    message->sessionSeqNum = endian::from_ipmi(header->sessSeqNum);
    message->isPacketEncrypted =
        ((header->payloadType & PAYLOAD_ENCRYPT_MASK) ? true : false);
    message->isPacketAuthenticated =
        ((header->payloadType & PAYLOAD_AUTH_MASK) ? true : false);

    auto payloadLen = endian::from_ipmi(header->payloadLength);

    if (message->isPacketAuthenticated)
    {
        if (!(internal::verifyPacketIntegrity(inPacket, message, payloadLen)))
        {
            throw std::runtime_error("Packet Integrity check failed");
        }
    }

    // Decrypt the payload if the payload is encrypted
    if (message->isPacketEncrypted)
    {
        // Assign the decrypted payload to the IPMI Message
        message->payload =
            internal::decryptPayload(inPacket, message, payloadLen);
    }
    else
    {
        message->payload.assign(inPacket.begin() + sizeof(SessionHeader_t),
                                inPacket.begin() + sizeof(SessionHeader_t) +
                                    payloadLen);
    }

    return message;
}

std::vector<uint8_t> flatten(std::shared_ptr<Message> outMessage,
                             std::shared_ptr<session::Session> session)
{
    std::vector<uint8_t> packet(sizeof(SessionHeader_t));

    SessionHeader_t* header = reinterpret_cast<SessionHeader_t*>(packet.data());
    header->base.version = parser::RMCP_VERSION;
    header->base.reserved = 0x00;
    header->base.rmcpSeqNum = parser::RMCP_SEQ;
    header->base.classOfMsg = parser::RMCP_MESSAGE_CLASS_IPMI;
    header->base.format.formatType =
        static_cast<uint8_t>(parser::SessionHeader::IPMI20);
    header->payloadType = static_cast<uint8_t>(outMessage->payloadType);
    header->sessId = endian::to_ipmi(outMessage->rcSessionID);

    // Add session sequence number
    internal::addSequenceNumber(packet, session);

    size_t payloadLen = 0;

    // Encrypt the payload if needed
    if (outMessage->isPacketEncrypted)
    {
        header->payloadType |= PAYLOAD_ENCRYPT_MASK;
        auto cipherPayload = internal::encryptPayload(outMessage);
        payloadLen = cipherPayload.size();
        header->payloadLength = endian::to_ipmi<uint16_t>(cipherPayload.size());

        // Insert the encrypted payload into the outgoing IPMI packet
        packet.insert(packet.end(), cipherPayload.begin(), cipherPayload.end());
    }
    else
    {
        header->payloadLength =
            endian::to_ipmi<uint16_t>(outMessage->payload.size());
        payloadLen = outMessage->payload.size();

        // Insert the Payload into the Packet
        packet.insert(packet.end(), outMessage->payload.begin(),
                      outMessage->payload.end());
    }

    if (outMessage->isPacketAuthenticated)
    {
        header = reinterpret_cast<SessionHeader_t*>(packet.data());
        header->payloadType |= PAYLOAD_AUTH_MASK;
        internal::addIntegrityData(packet, outMessage, payloadLen);
    }

    return packet;
}

namespace internal
{

void addSequenceNumber(std::vector<uint8_t>& packet,
                       std::shared_ptr<session::Session> session)
{
    SessionHeader_t* header = reinterpret_cast<SessionHeader_t*>(packet.data());

    if (header->sessId == session::sessionZero)
    {
        header->sessSeqNum = 0x00;
    }
    else
    {
        auto seqNum = session->sequenceNums.increment();
        header->sessSeqNum = endian::to_ipmi(seqNum);
    }
}

bool verifyPacketIntegrity(const std::vector<uint8_t>& packet,
                           const std::shared_ptr<Message> message,
                           size_t payloadLen)
{
    /*
     * Padding bytes are added to cause the number of bytes in the data range
     * covered by the AuthCode(Integrity Data) field to be a multiple of 4 bytes
     * .If present each integrity Pad byte is set to FFh. The following logic
     * calculates the number of padding bytes added in the IPMI packet.
     */
    auto paddingLen = 4 - ((payloadLen + 2) & 3);

    auto sessTrailerPos = sizeof(SessionHeader_t) + payloadLen + paddingLen;

    auto trailer = reinterpret_cast<const SessionTrailer_t*>(packet.data() +
                                                             sessTrailerPos);

    // Check trailer->padLength against paddingLen, both should match up,
    // return false if the lengths don't match
    if (trailer->padLength != paddingLen)
    {
        return false;
    }

    auto session = std::get<session::Manager&>(singletonPool)
                       .getSession(message->bmcSessionID);

    auto integrityAlgo = session->getIntegrityAlgo();

    // Check if Integrity data length is as expected, check integrity data
    // length is same as the length expected for the Integrity Algorithm that
    // was negotiated during the session open process.
    if ((packet.size() - sessTrailerPos - sizeof(SessionTrailer_t)) !=
        integrityAlgo->authCodeLength)
    {
        return false;
    }

    auto integrityIter = packet.cbegin();
    std::advance(integrityIter, sessTrailerPos + sizeof(SessionTrailer_t));

    // The integrity data is calculated from the AuthType/Format field up to and
    // including the field that immediately precedes the AuthCode field itself.
    size_t length = packet.size() - integrityAlgo->authCodeLength -
                    message::parser::RMCP_SESSION_HEADER_SIZE;

    return integrityAlgo->verifyIntegrityData(packet, length, integrityIter);
}

void addIntegrityData(std::vector<uint8_t>& packet,
                      const std::shared_ptr<Message> message, size_t payloadLen)
{
    // The following logic calculates the number of padding bytes to be added to
    // IPMI packet. If needed each integrity Pad byte is set to FFh.
    auto paddingLen = 4 - ((payloadLen + 2) & 3);
    packet.insert(packet.end(), paddingLen, 0xFF);

    packet.resize(packet.size() + sizeof(SessionTrailer_t));

    auto trailer = reinterpret_cast<SessionTrailer_t*>(
        packet.data() + packet.size() - sizeof(SessionTrailer_t));

    trailer->padLength = paddingLen;
    trailer->nextHeader = parser::RMCP_MESSAGE_CLASS_IPMI;

    auto session = std::get<session::Manager&>(singletonPool)
                       .getSession(message->bmcSessionID);

    auto integrityData =
        session->getIntegrityAlgo()->generateIntegrityData(packet);

    packet.insert(packet.end(), integrityData.begin(), integrityData.end());
}

std::vector<uint8_t> decryptPayload(const std::vector<uint8_t>& packet,
                                    const std::shared_ptr<Message> message,
                                    size_t payloadLen)
{
    auto session = std::get<session::Manager&>(singletonPool)
                       .getSession(message->bmcSessionID);

    return session->getCryptAlgo()->decryptPayload(
        packet, sizeof(SessionHeader_t), payloadLen);
}

std::vector<uint8_t> encryptPayload(std::shared_ptr<Message> message)
{
    auto session = std::get<session::Manager&>(singletonPool)
                       .getSession(message->bmcSessionID);

    return session->getCryptAlgo()->encryptPayload(message->payload);
}

} // namespace internal

} // namespace ipmi20parser

} // namespace message
