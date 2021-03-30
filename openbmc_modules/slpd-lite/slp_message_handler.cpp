#include "config.h"

#include "endian.hpp"
#include "slp.hpp"
#include "slp_meta.hpp"

#include <arpa/inet.h>
#include <dirent.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <string.h>

#include <algorithm>

namespace slp
{
namespace handler
{

namespace internal
{

buffer prepareHeader(const Message& req)
{
    uint8_t length =
        slp::header::MIN_LEN +        /* 14 bytes for header     */
        req.header.langtag.length() + /* Actual length of lang tag */
        slp::response::SIZE_ERROR;    /*  2 bytes for error code */

    buffer buff(length, 0);

    buff[slp::header::OFFSET_VERSION] = req.header.version;

    // will increment the function id from 1 as reply
    buff[slp::header::OFFSET_FUNCTION] = req.header.functionID + 1;

    std::copy_n(&length, slp::header::SIZE_LENGTH,
                buff.data() + slp::header::OFFSET_LENGTH);

    auto flags = endian::to_network(req.header.flags);

    std::copy_n((uint8_t*)&flags, slp::header::SIZE_FLAGS,
                buff.data() + slp::header::OFFSET_FLAGS);

    std::copy_n(req.header.extOffset.data(), slp::header::SIZE_EXT,
                buff.data() + slp::header::OFFSET_EXT);

    auto xid = endian::to_network(req.header.xid);

    std::copy_n((uint8_t*)&xid, slp::header::SIZE_XID,
                buff.data() + slp::header::OFFSET_XID);

    uint16_t langtagLen = req.header.langtag.length();
    langtagLen = endian::to_network(langtagLen);
    std::copy_n((uint8_t*)&langtagLen, slp::header::SIZE_LANG,
                buff.data() + slp::header::OFFSET_LANG_LEN);

    std::copy_n((uint8_t*)req.header.langtag.c_str(),
                req.header.langtag.length(),
                buff.data() + slp::header::OFFSET_LANG);
    return buff;
}

std::tuple<int, buffer> processSrvTypeRequest(const Message& req)
{

    /*
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |      Service Location header (function = SrvTypeRply = 10)    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |           Error Code          |    length of <srvType-list>   |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       <srvtype--list>                         \
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */

    buffer buff;

    // read the slp service info from conf and create the service type string
    slp::handler::internal::ServiceList svcList =
        slp::handler::internal::readSLPServiceInfo();
    if (svcList.size() <= 0)
    {
        buff.resize(0);
        std::cerr << "SLP unable to read the service info\n";
        return std::make_tuple((int)slp::Error::INTERNAL_ERROR, buff);
    }

    std::string service;
    bool firstIteration = true;
    for_each(svcList.cbegin(), svcList.cend(),
             [&service, &firstIteration](const auto& svc) {
                 if (firstIteration == true)
                 {
                     service = svc.first;
                     firstIteration = false;
                 }
                 else
                 {
                     service += ",";
                     service += svc.first;
                 }
             });

    buff = prepareHeader(req);

    /* Need to modify the length and the function type field of the header
     * as it is dependent on the handler of the service */

    std::cout << "service=" << service.c_str() << "\n";

    uint8_t length = buff.size() + /* 14 bytes header + length of langtag */
                     slp::response::SIZE_ERROR +   /* 2 byte err code */
                     slp::response::SIZE_SERVICE + /* 2 byte srvtype len */
                     service.length();

    buff.resize(length);

    std::copy_n(&length, slp::header::SIZE_LENGTH,
                buff.data() + slp::header::OFFSET_LENGTH);

    /* error code is already set to 0 moving to service type len */

    uint16_t serviceTypeLen = service.length();
    serviceTypeLen = endian::to_network(serviceTypeLen);

    std::copy_n((uint8_t*)&serviceTypeLen, slp::response::SIZE_SERVICE,
                buff.data() + slp::response::OFFSET_SERVICE_LEN);

    /* service type data */
    std::copy_n((uint8_t*)service.c_str(), service.length(),
                (buff.data() + slp::response::OFFSET_SERVICE));

    return std::make_tuple(slp::SUCCESS, buff);
}

std::tuple<int, buffer> processSrvRequest(const Message& req)
{

    /*
          Service Reply
          0                   1                   2                   3
          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |        Service Location header (function = SrvRply = 2)       |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |        Error Code             |        URL Entry count        |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |       <URL Entry 1>          ...       <URL Entry N>          \
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

         URL Entry
          0                   1                   2                   3
          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |   Reserved    |          Lifetime             |   URL Length  |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |URL len, contd.|            URL (variable length)              \
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |# of URL auths |            Auth. blocks (if any)              \
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */

    buffer buff;
    // Get all the services which are registered
    slp::handler::internal::ServiceList svcList =
        slp::handler::internal::readSLPServiceInfo();
    if (svcList.size() <= 0)
    {
        buff.resize(0);
        std::cerr << "SLP unable to read the service info\n";
        return std::make_tuple((int)slp::Error::INTERNAL_ERROR, buff);
    }

    // return error if serice type doesn't match
    auto& svcName = req.body.srvrqst.srvType;
    auto svcIt = svcList.find(svcName);
    if (svcIt == svcList.end())
    {
        buff.resize(0);
        std::cerr << "SLP unable to find the service=" << svcName << "\n";
        return std::make_tuple((int)slp::Error::INTERNAL_ERROR, buff);
    }
    // Get all the interface address
    auto ifaddrList = slp::handler::internal::getIntfAddrs();
    if (ifaddrList.size() <= 0)
    {
        buff.resize(0);
        std::cerr << "SLP unable to read the interface address\n";
        return std::make_tuple((int)slp::Error::INTERNAL_ERROR, buff);
    }

    buff = prepareHeader(req);
    // Calculate the length and resize the buffer
    uint8_t length = buff.size() + /* 14 bytes header + length of langtag */
                     slp::response::SIZE_ERROR +    /* 2 bytes error code */
                     slp::response::SIZE_URL_COUNT; /* 2 bytes srvtype len */

    buff.resize(length);

    // Populate the url count
    uint16_t urlCount = ifaddrList.size();
    urlCount = endian::to_network(urlCount);

    std::copy_n((uint8_t*)&urlCount, slp::response::SIZE_URL_COUNT,
                buff.data() + slp::response::OFFSET_URL_ENTRY);

    // Find the service
    const slp::ConfigData& svc = svcIt->second;
    // Populate the URL Entries
    auto pos = slp::response::OFFSET_URL_ENTRY + slp::response::SIZE_URL_COUNT;
    for (const auto& addr : ifaddrList)
    {
        std::string url =
            svc.name + ':' + svc.type + "//" + addr + ',' + svc.port;

        buff.resize(buff.size() + slp::response::SIZE_URL_ENTRY + url.length());

        uint8_t reserved = 0;
        uint16_t auth = 0;
        uint16_t lifetime = endian::to_network<uint16_t>(slp::LIFETIME);
        uint16_t urlLength = url.length();

        std::copy_n((uint8_t*)&reserved, slp::response::SIZE_RESERVED,
                    buff.data() + pos);

        pos += slp::response::SIZE_RESERVED;

        std::copy_n((uint8_t*)&lifetime, slp::response::SIZE_LIFETIME,
                    buff.data() + pos);

        pos += slp::response::SIZE_LIFETIME;

        urlLength = endian::to_network(urlLength);
        std::copy_n((uint8_t*)&urlLength, slp::response::SIZE_URLLENGTH,
                    buff.data() + pos);
        pos += slp::response::SIZE_URLLENGTH;

        std::copy_n((uint8_t*)url.c_str(), url.length(), buff.data() + pos);
        pos += url.length();

        std::copy_n((uint8_t*)&auth, slp::response::SIZE_AUTH,
                    buff.data() + pos);
        pos += slp::response::SIZE_AUTH;
    }
    uint8_t packetLength = buff.size();
    std::copy_n((uint8_t*)&packetLength, slp::header::SIZE_VERSION,
                buff.data() + slp::header::OFFSET_LENGTH);

    return std::make_tuple((int)slp::SUCCESS, buff);
}

std::list<std::string> getIntfAddrs()
{
    std::list<std::string> addrList;

    struct ifaddrs* ifaddr;
    // attempt to fill struct with ifaddrs
    if (getifaddrs(&ifaddr) == -1)
    {
        return addrList;
    }

    slp::deleted_unique_ptr<ifaddrs> ifaddrPtr(
        ifaddr, [](ifaddrs* addr) { freeifaddrs(addr); });

    ifaddr = nullptr;

    for (ifaddrs* ifa = ifaddrPtr.get(); ifa != nullptr; ifa = ifa->ifa_next)
    {
        // walk interfaces
        if (ifa->ifa_addr == nullptr)
        {
            continue;
        }

        // get only INET interfaces not ipv6
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            // if loopback, or not running ignore
            if ((ifa->ifa_flags & IFF_LOOPBACK) ||
                !(ifa->ifa_flags & IFF_RUNNING))
            {
                continue;
            }

            char tmp[INET_ADDRSTRLEN] = {0};

            inet_ntop(AF_INET,
                      &(((struct sockaddr_in*)(ifa->ifa_addr))->sin_addr), tmp,
                      sizeof(tmp));
            addrList.emplace_back(tmp);
        }
    }

    return addrList;
}

slp::handler::internal::ServiceList readSLPServiceInfo()
{
    using namespace std::string_literals;
    slp::handler::internal::ServiceList svcLst;
    slp::ConfigData service;
    struct dirent* dent = nullptr;

    // Open the services dir and get the service info
    // from service files.
    // Service File format would be "ServiceName serviceType Port"
    DIR* dir = opendir(SERVICE_DIR);
    // wrap the pointer into smart pointer.
    slp::deleted_unique_ptr<DIR> dirPtr(dir, [](DIR* dir) {
        if (!dir)
        {
            closedir(dir);
        }
    });
    dir = nullptr;

    if (dirPtr.get())
    {
        while ((dent = readdir(dirPtr.get())) != NULL)
        {
            if (dent->d_type == DT_REG) // regular file
            {
                auto absFileName = std::string(SERVICE_DIR) + dent->d_name;
                std::ifstream readFile(absFileName);
                readFile >> service;
                service.name = "service:"s + service.name;
                svcLst.emplace(service.name, service);
            }
        }
    }
    return svcLst;
}
} // namespace internal

std::tuple<int, buffer> processRequest(const Message& msg)
{
    int rc = slp::SUCCESS;
    buffer resp(0);
    std::cout << "SLP Processing Request=" << msg.header.functionID << "\n";

    switch (msg.header.functionID)
    {
        case (uint8_t)slp::FunctionType::SRVTYPERQST:
            std::tie(rc, resp) =
                slp::handler::internal::processSrvTypeRequest(msg);
            break;
        case (uint8_t)slp::FunctionType::SRVRQST:
            std::tie(rc, resp) = slp::handler::internal::processSrvRequest(msg);
            break;
        default:
            rc = (uint8_t)slp::Error::MSG_NOT_SUPPORTED;
    }
    return std::make_tuple(rc, resp);
}

buffer processError(const Message& req, uint8_t err)
{
    buffer buff;
    buff = slp::handler::internal::prepareHeader(req);

    std::copy_n(&err, slp::response::SIZE_ERROR,
                buff.data() + slp::response::OFFSET_ERROR);
    return buff;
}
} // namespace handler
} // namespace slp
