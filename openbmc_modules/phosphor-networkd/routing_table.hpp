#pragma once

#include <asm/types.h>
#include <linux/netlink.h>
#include <sys/socket.h>

#include <iostream>
#include <list>
#include <map>
#include <string>

namespace phosphor
{
namespace network
{
namespace route
{
constexpr auto BUFSIZE = 4096;

struct Entry
{
    // destination network
    std::string destination;
    // gateway for this network.
    std::string gateway;
    // interface for this route
    std::string interface;
    Entry(const std::string& dest, const std::string& gtw,
          const std::string& intf) :
        destination(dest),
        gateway(gtw), interface(intf)
    {
    }

    bool operator==(const Entry& rhs)
    {
        return this->destination == rhs.destination &&
               this->gateway == rhs.gateway && this->interface == rhs.interface;
    }
};

// Map of network address and the route entry
using Map = std::map<std::string, struct Entry>;

class Table
{
  public:
    Table();
    ~Table() = default;
    Table(const Table&) = default;
    Table& operator=(const Table&) = default;
    Table(Table&&) = default;
    Table& operator=(Table&&) = default;

    /**
     * @brief gets the list of routes.
     *
     * @returns list of routes.
     */
    Map getRoutes();

    /**
     * @brief gets the default v4 gateway.
     *
     * @returns the default v4 gateway.
     */
    std::string getDefaultGateway() const
    {
        return defaultGateway;
    };

    /**
     * @brief gets the default v6 gateway.
     *
     * @returns the default v6 gateway.
     */
    std::string getDefaultGateway6() const
    {
        return defaultGateway6;
    };

  private:
    /**
     * @brief read the routing data from the socket and fill the buffer.
     *
     * @param[in] bufPtr - unique pointer to confidentiality algorithm
     *                     instance
     */
    int readNetLinkSock(int sockFd, std::array<char, BUFSIZE>& buff);
    /**
     * @brief Parse the route and add it to the route list.
     *
     * @param[in] nlHdr - net link message header.
     */
    void parseRoutes(const struct nlmsghdr* nlHdr);

    std::string defaultGateway;  // default gateway
    std::string defaultGateway6; // default gateway
    Map routeList;               // List of routes
};

} // namespace route
} // namespace network
} // namespace phosphor
