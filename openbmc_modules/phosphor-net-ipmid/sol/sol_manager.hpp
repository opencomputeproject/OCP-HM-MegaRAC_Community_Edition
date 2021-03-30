#pragma once

#include "console_buffer.hpp"
#include "session.hpp"
#include "sol_context.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <cstddef>
#include <map>
#include <memory>
#include <string>

namespace sol
{

constexpr size_t MAX_PAYLOAD_SIZE = 255;
constexpr uint8_t MAJOR_VERSION = 0x01;
constexpr uint8_t MINOR_VERSION = 0x00;

constexpr char CONSOLE_SOCKET_PATH[] = "\0obmc-console";
constexpr size_t CONSOLE_SOCKET_PATH_LEN = sizeof(CONSOLE_SOCKET_PATH) - 1;

constexpr uint8_t accIntervalFactor = 5;
constexpr uint8_t retryIntervalFactor = 10;

using Instance = uint8_t;

using namespace std::chrono_literals;

/** @class Manager
 *
 *  Manager class acts a manager for the SOL payload instances and provides
 *  interfaces to start a payload instance, stop a payload instance and get
 *  reference to the context object.
 */
class Manager
{
  public:
    /** @brief SOL Payload Instance is the key for the map, the value is the
     *         SOL context.
     */
    using SOLPayloadMap = std::map<Instance, std::shared_ptr<Context>>;

    Manager() = delete;
    ~Manager() = default;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = default;
    Manager& operator=(Manager&&) = default;

    Manager(std::shared_ptr<boost::asio::io_context> io) : io(io)
    {
    }

    /** @brief io context to add events to */
    std::shared_ptr<boost::asio::io_context> io;

    /** @brief Host Console Buffer. */
    ConsoleData dataBuffer;

    /** @brief Set in Progress.
     *
     *  This parameter is used to indicate when any of the SOL parameters
     *  are being updated, and when the changes are completed. The bit is
     *  primarily provided to alert software than some other software or
     *  utility is in the process of making changes to the data. This field
     *  is initialized to set complete.
     */
    uint8_t progress = 0;

    /** @brief SOL enable
     *
     *  This controls whether the SOL payload can be activated. By default
     *  the SOL is enabled.
     */
    bool enable = true;

    /** @brief SOL payload encryption.
     *
     *  Force encryption: if the cipher suite for the session supports
     *  encryption, then this setting will force the use of encryption for
     *  all SOL payload data. Encryption controlled by remote console:
     *  Whether SOL packets are encrypted or not is selectable by the remote
     *  console at the time the payload is activated. The default is force
     *  encryption.
     */
    bool forceEncrypt = true;

    /** @brief SOL payload authentication.
     *
     *  Force authentication: if the cipher suite for the session supports
     *  authentication, then this setting will force the use of  for
     *  authentication for all SOL payload data. Authentication controlled
     *  by remote console: Note that for the standard Cipher Suites,
     *  if encryption is used authentication must also be used. Therefore,
     *  while encryption is being used software will not be able to select
     *  using unauthenticated payloads.
     */
    bool forceAuth = true;

    /** @brief SOL privilege level.
     *
     *  Sets the minimum operating privilege level that is required to be
     *  able to activate SOL using the Activate Payload command.
     */
    session::Privilege solMinPrivilege = session::Privilege::USER;

    /** @brief Character Accumulate Interval
     *
     *  This sets the typical amount of time that the BMC will wait before
     *  transmitting a partial SOL character data packet. (Where a partial
     *  packet is defined as a packet that has fewer characters to transmit
     *  than the number of characters specified by the character send
     *  threshold. This parameter can be modified by the set SOL
     *  configuration parameters command. The SOL configuration parameter,
     *  Character Accumulate Interval is 5 ms increments, 1-based value. The
     *  parameter value is accumulateInterval/5. The accumulateInterval
     *  needs to be a multiple of 5.
     */
    std::chrono::milliseconds accumulateInterval = 100ms;

    /** @brief Character Send Threshold
     *
     *  The BMC will automatically send an SOL character data packet
     *  containing this number of characters as soon as this number of
     *  characters (or greater) has been accepted from the baseboard serial
     *  controller into the BMC. This provides a mechanism to tune the
     *  buffer to reduce latency to when the first characters are received
     *  after an idle interval. In the degenerate case, setting this value
     *  to a ‘1’ would cause the BMC to send a packet as soon as the first
     *  character was received. This parameter can be modified by the set
     *  SOL configuration parameters command.
     */
    uint8_t sendThreshold = 1;

    /** @brief Retry Count
     *
     *  1-based. 0 = no retries after packet is transmitted. Packet will be
     *  dropped if no ACK/NACK received by time retries expire. The maximum
     *  value for retry count is 7. This parameter can be modified by the
     *  set SOL configuration parameters command.
     */
    uint8_t retryCount = 7;

    /** @brief Retry Interval
     *
     *  Sets the time that the BMC will wait before the first retry and the
     *  time between retries when sending SOL packets to the remote console.
     *  This parameter can be modified by the set SOL configuration
     *  parameters command. The SOL configuration parameter Retry Interval
     *  is 10 ms increments, 1-based value. The parameter value is
     *  retryInterval/10. The retryInterval needs to be a multiple of 10.
     */
    std::chrono::milliseconds retryInterval = 100ms;

    /** @brief Channel Number
     *
     *  This parameter indicates which IPMI channel is being used for the
     *  communication parameters (e.g. IP address, MAC address) for the SOL
     *  Payload. Typically, these parameters will come from the same channel
     *  that the Activate Payload command for SOL was accepted over. The
     *  network channel number is defaulted to 1.
     */
    uint8_t channel = 1;

    /** @brief Add host console I/O event source to the event loop.  */
    void startHostConsole();

    /** @brief Remove host console I/O event source. */
    void stopHostConsole();

    /** @brief Start a SOL payload instance.
     *
     *  Starting a payload instance involves creating the context object,
     *  add the accumulate interval timer and retry interval timer to the
     *  event loop.
     *
     *  @param[in] payloadInstance - SOL payload instance.
     *  @param[in] sessionID - BMC session ID.
     */
    void startPayloadInstance(uint8_t payloadInstance,
                              session::SessionID sessionID);

    /** @brief Stop SOL payload instance.
     *
     *  Stopping a payload instance involves stopping and removing the
     *  accumulate interval timer and retry interval timer from the event
     *  loop, delete the context object.
     *
     *  @param[in] payloadInstance - SOL payload instance
     */
    void stopPayloadInstance(uint8_t payloadInstance);

    /** @brief Get SOL Context by Payload Instance.
     *
     *  @param[in] payloadInstance - SOL payload instance.
     *
     *  @return reference to the SOL payload context.
     */
    Context& getContext(uint8_t payloadInstance)
    {
        auto iter = payloadMap.find(payloadInstance);

        if (iter != payloadMap.end())
        {
            return *(iter->second);
        }

        std::string msg = "Invalid SOL payload instance " + payloadInstance;
        throw std::runtime_error(msg.c_str());
    }

    /** @brief Get SOL Context by Session ID.
     *
     *  @param[in] sessionID - IPMI Session ID.
     *
     *  @return reference to the SOL payload context.
     */
    Context& getContext(session::SessionID sessionID)
    {
        for (const auto& kv : payloadMap)
        {
            if (kv.second->sessionID == sessionID)
            {
                return *kv.second;
            }
        }

        std::string msg = "Invalid SOL SessionID " + sessionID;
        throw std::runtime_error(msg.c_str());
    }

    /** @brief Check if SOL payload is active.
     *
     *  @param[in] payloadInstance - SOL payload instance.
     *
     *  @return true if the instance is active and false it is not active.
     */
    auto isPayloadActive(uint8_t payloadInstance) const
    {
        return (0 != payloadMap.count(payloadInstance));
    }

    /** @brief Write data to the host console unix socket.
     *
     *  @param[in] input - Data from the remote console.
     *
     *  @return 0 on success and errno on failure.
     */
    int writeConsoleSocket(const std::vector<uint8_t>& input) const;

  private:
    SOLPayloadMap payloadMap;

    /** @brief Local stream socket for the host console. */
    std::unique_ptr<boost::asio::local::stream_protocol::socket> consoleSocket =
        nullptr;

    /** @brief Initialize the host console file descriptor. */
    void initConsoleSocket();

    /** @brief Handle incoming console data on the console socket */
    void consoleInputHandler();
};

} // namespace sol
