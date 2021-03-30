#include "libpldm/platform.h"
#include "libpldm/requester/pldm.h"

#include <CLI/CLI.hpp>

#include <array>
#include <iostream>

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

    // Open connection to MCTP socket
    int fd = pldm_open();
    if (-1 == fd)
    {
        std::cerr << "Failed to init mctp"
                  << "\n";
        return -1;
    }

    uint8_t* responseMsg = nullptr;
    size_t responseMsgSize{};
    // Send PLDM request msg and wait for response
    rc = pldm_send_recv(mctpEid, fd, requestMsg.data(), requestMsg.size(),
                        &responseMsg, &responseMsgSize);
    if (0 > rc)
    {
        std::cerr << "Failed to send message/receive response. RC = " << rc
                  << ", errno = " << errno << "\n";
        return -1;
    }
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg);
    std::cout << "Done. PLDM RC = " << std::hex << std::showbase
              << static_cast<uint16_t>(response->payload[0]) << std::endl;
    free(responseMsg);

    return 0;
}
