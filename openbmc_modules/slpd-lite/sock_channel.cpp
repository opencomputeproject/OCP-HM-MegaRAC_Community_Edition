#include "sock_channel.hpp"

#include <errno.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

namespace udpsocket
{

std::string Channel::getRemoteAddress() const
{
    char tmp[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET6, &address.inAddr.sin6_addr, tmp, sizeof(tmp));
    return std::string(tmp);
}

std::tuple<int, buffer> Channel::read()
{
    int rc = 0;
    int readSize = 0;
    ssize_t readDataLen = 0;
    buffer outBuffer(0);

    if (ioctl(sockfd, FIONREAD, &readSize) < 0)
    {
        std::cerr << "Channel::Read : ioctl failed with errno = " << errno;
        rc = -errno;
        return std::make_tuple(rc, std::move(outBuffer));
    }

    outBuffer.resize(readSize);
    auto bufferSize = outBuffer.size();
    auto outputPtr = outBuffer.data();

    address.addrSize = static_cast<socklen_t>(sizeof(address.inAddr));

    do
    {
        readDataLen = recvfrom(sockfd,             // File Descriptor
                               outputPtr,          // Buffer
                               bufferSize,         // Bytes requested
                               0,                  // Flags
                               &address.sockAddr,  // Address
                               &address.addrSize); // Address Length

        if (readDataLen == 0) // Peer has performed an orderly shutdown
        {
            outBuffer.resize(0);
            rc = -1;
        }
        else if (readDataLen < 0) // Error
        {
            rc = -errno;
            std::cerr << "Channel::Read : Receive Error Fd[" << sockfd << "]"
                      << "errno = " << rc << "\n";
            outBuffer.resize(0);
        }
    } while ((readDataLen < 0) && (-(rc) == EINTR));

    // Resize the vector to the actual data read from the socket
    outBuffer.resize(readDataLen);
    return std::make_tuple(rc, std::move(outBuffer));
}

int Channel::write(buffer& inBuffer)
{
    int rc = 0;
    auto outputPtr = inBuffer.data();
    auto bufferSize = inBuffer.size();
    auto spuriousWakeup = false;
    ssize_t writeDataLen = 0;
    timeval varTimeout = timeout;

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sockfd, &writeSet);

    do
    {
        spuriousWakeup = false;

        rc = select((sockfd + 1), nullptr, &writeSet, NULL, &varTimeout);

        if (rc > 0)
        {
            if (FD_ISSET(sockfd, &writeSet))
            {
                address.addrSize =
                    static_cast<socklen_t>(sizeof(address.inAddr));
                do
                {
                    writeDataLen =
                        sendto(sockfd,            // File Descriptor
                               outputPtr,         // Message
                               bufferSize,        // Length
                               MSG_NOSIGNAL,      // Flags
                               &address.sockAddr, // Destination Address
                               address.addrSize); // Address Length

                    if (writeDataLen < 0)
                    {
                        rc = -errno;
                        std::cerr
                            << "Channel::Write: Write failed with errno:" << rc
                            << "\n";
                    }
                    else if (static_cast<size_t>(writeDataLen) < bufferSize)
                    {
                        rc = -1;
                        std::cerr << "Channel::Write: Complete data not written"
                                     " to the socket\n";
                    }
                } while ((writeDataLen < 0) && (-(rc) == EINTR));
            }
            else
            {
                // Spurious wake up
                std::cerr << "Spurious wake up on select (writeset)\n";
                spuriousWakeup = true;
            }
        }
        else
        {
            if (rc == 0)
            {
                // Timed out
                rc = -1;
                std::cerr << "We timed out on select call (writeset)\n";
            }
            else
            {
                // Error
                rc = -errno;
                std::cerr << "select call (writeset) had an error : " << rc
                          << "\n";
            }
        }
    } while (spuriousWakeup);

    return rc;
}

} // namespace udpsocket
