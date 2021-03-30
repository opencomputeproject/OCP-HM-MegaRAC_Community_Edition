#pragma once

#include "console_buffer.hpp"
#include "session.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <cstddef>

namespace sol
{

/** @struct Outbound
 *
 *  Operation/Status in an outbound SOL payload format(BMC to Remote Console).
 */
struct Outbound
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t testMode : 2;        //!< Not supported.
    uint8_t breakDetected : 1;   //!< Not supported.
    uint8_t transmitOverrun : 1; //!< Not supported.
    uint8_t SOLDeactivating : 1; //!< 0 : SOL is active, 1 : SOL deactivated.
    uint8_t charUnavailable : 1; //!< 0 : Available, 1 : Unavailable.
    uint8_t ack : 1;             //!< 0 : ACK, 1 : NACK.
    uint8_t reserved : 1;        //!< Reserved.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved : 1;        //!< Reserved.
    uint8_t ack : 1;             //!< 0 : ACK, 1 : NACK.
    uint8_t charUnavailable : 1; //!< 0 : Available, 1 : Unavailable.
    uint8_t SOLDeactivating : 1; //!< 0 : SOL is active, 1 : SOL deactivated.
    uint8_t transmitOverrun : 1; //!< Not supported.
    uint8_t breakDetected : 1;   //!< Not supported.
    uint8_t testMode : 2;        //!< Not supported.
#endif
} __attribute__((packed));

/** @struct Inbound
 *
 *  Operation/Status in an Inbound SOL Payload format(Remote Console to BMC).
 */
struct Inbound
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t flushOut : 1;      //!< Not supported.
    uint8_t flushIn : 1;       //!< Not supported.
    uint8_t dcd : 1;           //!< Not supported.
    uint8_t cts : 1;           //!< Not supported.
    uint8_t generateBreak : 1; //!< Not supported.
    uint8_t ring : 1;          //!< Not supported.
    uint8_t ack : 1;           //!< 0 : ACK, 1 : NACK.
    uint8_t reserved : 1;      //!< Reserved.
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved : 1;      //!< Reserved.
    uint8_t ack : 1;           //!< 0 : ACK, 1 : NACK.
    uint8_t ring : 1;          //!< Not supported.
    uint8_t generateBreak : 1; //!< Not supported.
    uint8_t cts : 1;           //!< Not supported.
    uint8_t dcd : 1;           //!< Not supported.
    uint8_t flushIn : 1;       //!< Not supported.
    uint8_t flushOut : 1;      //!< Not supported.
#endif
} __attribute__((packed));

/** @struct Payload
 *
 *  SOL Payload Data Format.The following fields make up the SOL payload in an
 *  RMCP+ packet, followed by the console character data.
 */
struct Payload
{
    uint8_t packetSeqNum;      //!< Packet sequence number
    uint8_t packetAckSeqNum;   //!< Packet ACK/NACK sequence number
    uint8_t acceptedCharCount; //!< Accepted character count
    union
    {
        uint8_t operation;            //!< Operation/Status
        struct Outbound outOperation; //!< BMC to Remote Console
        struct Inbound inOperation;   //!< Remote Console to BMC
    };
} __attribute__((packed));

namespace internal
{

/** @struct SequenceNumbers
 *
 *  SOL sequence numbers. At the session level, SOL Payloads share the session
 *  sequence numbers for authenticated and unauthenticated packets with other
 *  packets under the IPMI session. At the payload level, SOL packets include
 *  their own message sequence numbers that are used for tracking missing and
 *  retried SOL messages. The sequence numbers must be non-zero. Retried
 *  packets use the same sequence number as the first packet.
 */
struct SequenceNumbers
{
    static constexpr uint8_t MAX_SOL_SEQUENCE_NUMBER = 0x10;

    /** @brief Get the SOL sequence number.
     *
     *  @param[in] inbound - true for inbound sequence number and false for
     *                       outbound sequence number
     *
     *  @return sequence number
     */
    auto get(bool inbound = true) const
    {
        return inbound ? in : out;
    }

    /** @brief Increment the inbound SOL sequence number. */
    void incInboundSeqNum()
    {
        if ((++in) == MAX_SOL_SEQUENCE_NUMBER)
        {
            in = 1;
        }
    }

    /** @brief Increment the outbound SOL sequence number.
     *
     *  @return outbound sequence number to populate the SOL payload.
     */
    auto incOutboundSeqNum()
    {
        if ((++out) == MAX_SOL_SEQUENCE_NUMBER)
        {
            out = 1;
        }

        return out;
    }

  private:
    uint8_t in = 1;  //!< Inbound sequence number.
    uint8_t out = 0; //!< Outbound sequence number, since the first
                     //!< operation is increment, it is initialised to 0
};

} // namespace internal

/** @class Context
 *
 *  Context keeps the state of the SOL session. The information needed to
 *  maintain the state of the SOL is part of this class. This class provides
 *  interfaces to handle incoming SOL payload, send response and send outbound
 *  SOL payload.
 */
class Context : public std::enable_shared_from_this<Context>
{
  public:
    Context() = delete;
    ~Context() = default;
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;

    /** @brief Context Factory
     *
     *  This is called by the SOL Manager when a SOL payload instance is
     *  started for the activate payload command. Its purpose is to be able
     *  to perform post-creation tasks on the object without changing the
     *  code flow
     *
     *  @param[in] io  - boost::asio io context for event scheduling.
     *  @param[in] maxRetryCount  - Retry count max value.
     *  @param[in] sendThreshold - Character send threshold.
     *  @param[in] instance - SOL payload instance.
     *  @param[in] sessionID - BMC session ID.
     */
    static std::shared_ptr<Context>
        makeContext(std::shared_ptr<boost::asio::io_context> io,
                    uint8_t maxRetryCount, uint8_t sendThreshold,
                    uint8_t instance, session::SessionID sessionID);

    /** @brief Context Constructor.
     *
     *  This should only be used by the Context factory makeContext
     *  or the accumulate timer will not be initialized properly
     *
     *  @param[in] io  - boost::asio io context for event scheduling.
     *  @param[in] maxRetryCount  - Retry count max value.
     *  @param[in] sendThreshold - Character send threshold.
     *  @param[in] instance - SOL payload instance.
     *  @param[in] sessionID - BMC session ID.
     */
    Context(std::shared_ptr<boost::asio::io_context> io, uint8_t maxRetryCount,
            uint8_t sendThreshold, uint8_t instance,
            session::SessionID sessionID);

    static constexpr auto clear = true;
    static constexpr auto noClear = false;

    /** @brief accumulate timer */
    boost::asio::steady_timer accumulateTimer;

    /** @brief retry timer */
    boost::asio::steady_timer retryTimer;

    /** @brief Retry count max value. */
    const uint8_t maxRetryCount = 0;

    /** @brief Retry counter. */
    uint8_t retryCounter = 0;

    /** @brief Character send threshold. */
    const uint8_t sendThreshold = 0;

    /** @brief SOL payload instance. */
    const uint8_t payloadInstance = 0;

    /** @brief Session ID. */
    const session::SessionID sessionID = 0;

    /** @brief session pointer
     */
    std::shared_ptr<session::Session> session;

    /** @brief enable/disable accumulate timer
     *
     *  The timeout interval is managed by the SOL Manager;
     *  this function only enables or disable the timer
     *
     *  @param[in] enable - enable(true) or disable(false) accumulation timer
     */
    void enableAccumulateTimer(bool enable);

    /** @brief enable/disable retry timer
     *
     *  The timeout interval is managed by the SOL Manager;
     *  this function only enables or disable the timer
     *
     *  @param[in] enable - enable(true) or disable(false) retry timer
     */
    void enableRetryTimer(bool enable);

    /** @brief Process the Inbound SOL payload.
     *
     *  The SOL payload from the remote console is processed and the
     *  acknowledgment handling is done.
     *
     *  @param[in] seqNum - Packet sequence number.
     *  @param[in] ackSeqNum - Packet ACK/NACK sequence number.
     *  @param[in] count - Accepted character count.
     *  @param[in] operation - ACK is false, NACK is true
     *  @param[in] input - Incoming SOL character data.
     */
    void processInboundPayload(uint8_t seqNum, uint8_t ackSeqNum, uint8_t count,
                               bool status, const std::vector<uint8_t>& input);

    /** @brief Send the outbound SOL payload.
     *
     *  @return zero on success and negative value if condition for sending
     *          the payload fails.
     */
    int sendOutboundPayload();

    /** @brief Resend the SOL payload.
     *
     *  @param[in] clear - if true then send the payload and clear the
     *                     cached payload, if false only send the payload.
     */
    void resendPayload(bool clear);

    /** @brief accumlate timer handler called by timer */
    void charAccTimerHandler();

    /** @brief retry timer handler called by timer */
    void retryTimerHandler();

  private:
    /** @brief Expected character count.
     *
     *  Expected Sequence number and expected character count is set before
     *  sending the SOL payload. The check is done against these values when
     *  an incoming SOL payload is received.
     */
    size_t expectedCharCount = 0;

    /** @brief Inbound and Outbound sequence numbers. */
    internal::SequenceNumbers seqNums;

    /** @brief Copy of the last sent SOL payload.
     *
     *  A copy of the SOL payload is kept here, so that when a retry needs
     *  to be attempted the payload is sent again.
     */
    std::vector<uint8_t> payloadCache;

    /**
     * @brief Send Response for Incoming SOL payload.
     *
     * @param[in] ackSeqNum - Packet ACK/NACK Sequence Number.
     * @param[in] count - Accepted Character Count.
     * @param[in] ack - Set ACK/NACK in the Operation.
     */
    void prepareResponse(uint8_t ackSeqNum, uint8_t count, bool ack);

    /** @brief Send the outgoing SOL payload.
     *
     *  @param[in] out - buffer containing the SOL payload.
     */
    void sendPayload(const std::vector<uint8_t>& out) const;
};

} // namespace sol
