#include "libpldm/base.h"
#include "libpldm/platform.h"
#include "libpldm/requester/pldm.h"

#include <CLI/CLI.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/io.hpp>

#include <array>
#include <iostream>

using namespace sdeventplus;
using namespace sdeventplus::source;

int main(int argc, char** argv)
{
    CLI::App app{"Send PLDM command SetStateEffecterStates"};
    uint8_t mctpEid{};
    app.add_option("-m,--mctp_eid", mctpEid, "MCTP EID")->required();
    uint16_t effecterId{};
    app.add_option("-e,--effecter", effecterId, "Effecter Id")->required();
    uint8_t state{};
    app.add_option("-s,--state", state, "New state value")->required();
    CLI11_PARSE(app, argc, argv);

    // Encode PLDM Request message
    uint8_t effecterCount = 1;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + sizeof(effecterId) +
                            sizeof(effecterCount) +
                            sizeof(set_effecter_state_field)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    set_effecter_state_field stateField{PLDM_REQUEST_SET, state};
    auto rc = encode_set_state_effecter_states_req(0, effecterId, effecterCount,
                                                   &stateField, request);
    if (rc != PLDM_SUCCESS)
    {
        std::cerr << "Message encode failure. PLDM error code = " << std::hex
                  << std::showbase << rc << "\n";
        return -1;
    }

    // Get fd of MCTP socket
    int fd = pldm_open();
    if (-1 == fd)
    {
        std::cerr << "Failed to init mctp"
                  << "\n";
        return -1;
    }

    // Create event loop and add a callback to handle EPOLLIN on fd
    auto event = Event::get_default();
    auto callback = [=](IO& io, int fd, uint32_t revents) {
        if (!(revents & EPOLLIN))
        {
            return;
        }

        uint8_t* responseMsg = nullptr;
        size_t responseMsgSize{};
        auto rc = pldm_recv(mctpEid, fd, request->hdr.instance_id, &responseMsg,
                            &responseMsgSize);
        if (!rc)
        {
            // We've got the response meant for the PLDM request msg that was
            // sent out
            io.set_enabled(Enabled::Off);
            pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg);
            std::cout << "Done. PLDM RC = " << std::hex << std::showbase
                      << static_cast<uint16_t>(response->payload[0])
                      << std::endl;
            free(responseMsg);
            exit(EXIT_SUCCESS);
        }
    };
    IO io(event, fd, EPOLLIN, std::move(callback));

    // Send PLDM Request message - pldm_send doesn't wait for response
    rc = pldm_send(mctpEid, fd, requestMsg.data(), requestMsg.size());
    if (0 > rc)
    {
        std::cerr << "Failed to send message/receive response. RC = " << rc
                  << ", errno = " << errno << "\n";
        return -1;
    }

    event.loop();

    return 0;
}
