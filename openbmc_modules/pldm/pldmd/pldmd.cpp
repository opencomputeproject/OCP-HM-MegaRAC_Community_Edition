#include "libpldm/base.h"
#include "libpldm/bios.h"
#include "libpldm/pdr.h"
#include "libpldm/platform.h"

#include "common/utils.hpp"
#include "dbus_impl_pdr.hpp"
#include "dbus_impl_requester.hpp"
#include "host-bmc/dbus_to_host_effecters.hpp"
#include "host-bmc/host_pdr_handler.hpp"
#include "invoker.hpp"
#include "libpldmresponder/base.hpp"
#include "libpldmresponder/bios.hpp"
#include "libpldmresponder/fru.hpp"
#include "libpldmresponder/platform.hpp"
#include "xyz/openbmc_project/PLDM/Event/server.hpp"

#include <err.h>
#include <getopt.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <sdeventplus/event.hpp>
#include <sdeventplus/source/io.hpp>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef OEM_IBM
#include "libpldmresponder/file_io.hpp"
#endif

constexpr uint8_t MCTP_MSG_TYPE_PLDM = 1;

using namespace pldm::responder;
using namespace pldm;
using namespace sdeventplus;
using namespace sdeventplus::source;

static Response processRxMsg(const std::vector<uint8_t>& requestMsg,
                             Invoker& invoker, dbus_api::Requester& requester)
{

    Response response;
    uint8_t eid = requestMsg[0];
    uint8_t type = requestMsg[1];
    pldm_header_info hdrFields{};
    auto hdr = reinterpret_cast<const pldm_msg_hdr*>(
        requestMsg.data() + sizeof(eid) + sizeof(type));
    if (PLDM_SUCCESS != unpack_pldm_header(hdr, &hdrFields))
    {
        std::cerr << "Empty PLDM request header \n";
    }
    else if (PLDM_RESPONSE != hdrFields.msg_type)
    {
        auto request = reinterpret_cast<const pldm_msg*>(hdr);
        size_t requestLen = requestMsg.size() - sizeof(struct pldm_msg_hdr) -
                            sizeof(eid) - sizeof(type);
        try
        {
            response = invoker.handle(hdrFields.pldm_type, hdrFields.command,
                                      request, requestLen);
        }
        catch (const std::out_of_range& e)
        {
            uint8_t completion_code = PLDM_ERROR_UNSUPPORTED_PLDM_CMD;
            response.resize(sizeof(pldm_msg_hdr));
            auto responseHdr = reinterpret_cast<pldm_msg_hdr*>(response.data());
            pldm_header_info header{};
            header.msg_type = PLDM_RESPONSE;
            header.instance = hdrFields.instance;
            header.pldm_type = hdrFields.pldm_type;
            header.command = hdrFields.command;
            auto result = pack_pldm_header(&header, responseHdr);
            if (PLDM_SUCCESS != result)
            {
                std::cerr << "Failed adding response header \n";
            }
            response.insert(response.end(), completion_code);
        }
    }
    else
    {
        requester.markFree(eid, hdr->instance_id);
    }
    return response;
}

void printBuffer(const std::vector<uint8_t>& buffer)
{
    std::ostringstream tempStream;
    tempStream << "Buffer Data: ";
    if (!buffer.empty())
    {
        for (int byte : buffer)
        {
            tempStream << std::setfill('0') << std::setw(2) << std::hex << byte
                       << " ";
        }
    }
    std::cout << tempStream.str().c_str() << std::endl;
}

void optionUsage(void)
{
    std::cerr << "Usage: pldmd [options]\n";
    std::cerr << "Options:\n";
    std::cerr
        << "  --verbose=<0/1>  0 - Disable verbosity, 1 - Enable verbosity\n";
    std::cerr << "Defaulted settings:  --verbose=0 \n";
}

int main(int argc, char** argv)
{

    bool verbose = false;
    static struct option long_options[] = {
        {"verbose", required_argument, 0, 'v'}, {0, 0, 0, 0}};

    auto argflag = getopt_long(argc, argv, "v:", long_options, nullptr);
    switch (argflag)
    {
        case 'v':
            switch (std::stoi(optarg))
            {
                case 0:
                    verbose = false;
                    break;
                case 1:
                    verbose = true;
                    break;
                default:
                    optionUsage();
                    break;
            }
            break;
        default:
            optionUsage();
            break;
    }

    /* Create local socket. */
    int returnCode = 0;
    int sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (-1 == sockfd)
    {
        returnCode = -errno;
        std::cerr << "Failed to create the socket, RC= " << returnCode << "\n";
        exit(EXIT_FAILURE);
    }

    auto event = Event::get_default();
    std::unique_ptr<pldm_pdr, decltype(&pldm_pdr_destroy)> pdrRepo(
        pldm_pdr_init(), pldm_pdr_destroy);
    std::unique_ptr<pldm_entity_association_tree,
                    decltype(&pldm_entity_association_tree_destroy)>
        entityTree(pldm_entity_association_tree_init(),
                   pldm_entity_association_tree_destroy);
    auto& bus = pldm::utils::DBusHandler::getBus();
    dbus_api::Requester dbusImplReq(bus, "/xyz/openbmc_project/pldm");
    std::unique_ptr<HostPDRHandler> hostPDRHandler;
    std::unique_ptr<pldm::host_effecters::HostEffecterParser>
        hostEffecterParser;
    auto dbusHandler = std::make_unique<DBusHandler>();
    auto hostEID = pldm::utils::readHostEID();
    if (hostEID)
    {
        hostPDRHandler = std::make_unique<HostPDRHandler>(
            sockfd, hostEID, event, pdrRepo.get(), entityTree.get(),
            dbusImplReq);
        hostEffecterParser =
            std::make_unique<pldm::host_effecters::HostEffecterParser>(
                &dbusImplReq, sockfd, pdrRepo.get(), dbusHandler.get(),
                HOST_JSONS_DIR, verbose);
    }

    Invoker invoker{};
    invoker.registerHandler(PLDM_BASE, std::make_unique<base::Handler>());
    invoker.registerHandler(PLDM_BIOS, std::make_unique<bios::Handler>());
    auto fruHandler = std::make_unique<fru::Handler>(
        FRU_JSONS_DIR, pdrRepo.get(), entityTree.get());
    // FRU table is built lazily when a FRU command or Get PDR command is
    // handled. To enable building FRU table, the FRU handler is passed to the
    // Platform handler.
    invoker.registerHandler(PLDM_PLATFORM,
                            std::make_unique<platform::Handler>(
                                dbusHandler.get(), PDR_JSONS_DIR,
                                EVENTS_JSONS_DIR, pdrRepo.get(),
                                hostPDRHandler.get(), fruHandler.get(), true));
    invoker.registerHandler(PLDM_FRU, std::move(fruHandler));

#ifdef OEM_IBM
    invoker.registerHandler(PLDM_OEM, std::make_unique<oem_ibm::Handler>());
#endif

    pldm::utils::CustomFD socketFd(sockfd);

    struct sockaddr_un addr
    {};
    addr.sun_family = AF_UNIX;
    const char path[] = "\0mctp-mux";
    memcpy(addr.sun_path, path, sizeof(path) - 1);
    int result = connect(socketFd(), reinterpret_cast<struct sockaddr*>(&addr),
                         sizeof(path) + sizeof(addr.sun_family) - 1);
    if (-1 == result)
    {
        returnCode = -errno;
        std::cerr << "Failed to connect to the socket, RC= " << returnCode
                  << "\n";
        exit(EXIT_FAILURE);
    }

    result = write(socketFd(), &MCTP_MSG_TYPE_PLDM, sizeof(MCTP_MSG_TYPE_PLDM));
    if (-1 == result)
    {
        returnCode = -errno;
        std::cerr << "Failed to send message type as pldm to mctp, RC= "
                  << returnCode << "\n";
        exit(EXIT_FAILURE);
    }

    dbus_api::Pdr dbusImplPdr(bus, "/xyz/openbmc_project/pldm", pdrRepo.get());
    sdbusplus::xyz::openbmc_project::PLDM::server::Event dbusImplEvent(
        bus, "/xyz/openbmc_project/pldm");
    auto callback = [verbose, &invoker, &dbusImplReq](IO& /*io*/, int fd,
                                                      uint32_t revents) {
        if (!(revents & EPOLLIN))
        {
            return;
        }

        // Outgoing message.
        struct iovec iov[2]{};

        // This structure contains the parameter information for the response
        // message.
        struct msghdr msg
        {};

        int returnCode = 0;
        ssize_t peekedLength = recv(fd, nullptr, 0, MSG_PEEK | MSG_TRUNC);
        if (0 == peekedLength)
        {
            std::cerr << "Socket has been closed \n";
        }
        else if (peekedLength <= -1)
        {
            returnCode = -errno;
            std::cerr << "recv system call failed, RC= " << returnCode << "\n";
        }
        else
        {
            std::vector<uint8_t> requestMsg(peekedLength);
            auto recvDataLength = recv(
                fd, static_cast<void*>(requestMsg.data()), peekedLength, 0);
            if (recvDataLength == peekedLength)
            {
                if (verbose)
                {
                    std::cout << "Received Msg" << std::endl;
                    printBuffer(requestMsg);
                }
                if (MCTP_MSG_TYPE_PLDM != requestMsg[1])
                {
                    // Skip this message and continue.
                    std::cerr << "Encountered Non-PLDM type message"
                              << "\n";
                }
                else
                {
                    // process message and send response
                    auto response =
                        processRxMsg(requestMsg, invoker, dbusImplReq);
                    if (!response.empty())
                    {
                        if (verbose)
                        {
                            std::cout << "Sending Msg" << std::endl;
                            printBuffer(response);
                        }

                        iov[0].iov_base = &requestMsg[0];
                        iov[0].iov_len =
                            sizeof(requestMsg[0]) + sizeof(requestMsg[1]);
                        iov[1].iov_base = response.data();
                        iov[1].iov_len = response.size();

                        msg.msg_iov = iov;
                        msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);

                        int result = sendmsg(fd, &msg, 0);
                        if (-1 == result)
                        {
                            returnCode = -errno;
                            std::cerr << "sendto system call failed, RC= "
                                      << returnCode << "\n";
                        }
                    }
                }
            }
            else
            {
                std::cerr
                    << "Failure to read peeked length packet. peekedLength= "
                    << peekedLength << " recvDataLength=" << recvDataLength
                    << "\n";
            }
        }
    };

    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    bus.request_name("xyz.openbmc_project.PLDM");
    IO io(event, socketFd(), EPOLLIN, std::move(callback));
    event.loop();

    result = shutdown(sockfd, SHUT_RDWR);
    if (-1 == result)
    {
        returnCode = -errno;
        std::cerr << "Failed to shutdown the socket, RC=" << returnCode << "\n";
        exit(EXIT_FAILURE);
    }
    exit(EXIT_FAILURE);
}
