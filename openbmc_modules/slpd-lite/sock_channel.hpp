#pragma once

#include <arpa/inet.h>
#include <unistd.h>

#include <string>
#include <tuple>
#include <vector>

namespace udpsocket
{

using buffer = std::vector<uint8_t>;
/** @class Channel
 *
 *  @brief Provides encapsulation for UDP socket operations like Read, Peek,
 *         Write, Remote peer's IP Address and Port.
 */
class Channel
{
  public:
    struct SockAddr_t
    {
        union
        {
            sockaddr sockAddr;
            sockaddr_in6 inAddr;
        };
        socklen_t addrSize;
    };

    /**
     * @brief Constructor
     *
     * Initialize the IPMI socket object with the socket descriptor
     *
     * @param [in] File Descriptor for the socket
     * @param [in] Timeout parameter for the select call
     *
     * @return None
     */
    Channel(int insockfd, timeval& inTimeout)
    {
        sockfd = insockfd;
        timeout = inTimeout;
    }

    /**
     * @brief Fetch the IP address of the remote peer
     *
     * Returns the IP address of the remote peer which is connected to this
     * socket
     *
     * @return IP address of the remote peer
     */
    std::string getRemoteAddress() const;

    /**
     * @brief Fetch the port number of the remote peer
     *
     * Returns the port number of the remote peer
     *
     * @return Port number
     *
     */
    auto getPort() const
    {
        return address.inAddr.sin6_port;
    }

    /**
     * @brief Read the incoming packet
     *
     * Reads the data available on the socket
     *
     * @return A tuple with return code and vector with the buffer
     *         In case of success, the vector is populated with the data
     *         available on the socket and return code is 0.
     *         In case of error, the return code is < 0 and vector is set
     *         to size 0.
     */
    std::tuple<int, buffer> read();

    /**
     *  @brief Write the outgoing packet
     *
     *  Writes the data in the vector to the socket
     *
     *  @param [in] inBuffer
     *      The vector would be the buffer of data to write to the socket.
     *
     *  @return In case of success the return code is 0 and return code is
     *          < 0 in case of failure.
     */
    int write(buffer& inBuffer);

    ~Channel() = default;
    Channel(const Channel& right) = delete;
    Channel& operator=(const Channel& right) = delete;
    Channel(Channel&&) = default;
    Channel& operator=(Channel&&) = default;

  private:
    /*
     * The socket descriptor is the UDP server socket for the IPMI port.
     * The same socket descriptor is used for multiple ipmi clients and the
     * life of the descriptor is lifetime of the net-ipmid server. So we
     * do not need to close the socket descriptor in the cleanup of the
     * udpsocket class.
     */
    int sockfd;
    SockAddr_t address;
    timeval timeout;
};

} // namespace udpsocket
