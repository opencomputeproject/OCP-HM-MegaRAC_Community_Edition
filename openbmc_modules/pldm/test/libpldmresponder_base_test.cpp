#include "libpldm/base.h"

#include "libpldmresponder/base.hpp"

#include <string.h>

#include <array>

#include <gtest/gtest.h>

using namespace pldm::responder;

TEST(GetPLDMTypes, testGoodRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr)> requestPayload{};
    auto request = reinterpret_cast<pldm_msg*>(requestPayload.data());
    // payload length will be 0 in this case
    size_t requestPayloadLength = 0;
    base::Handler handler;
    auto response = handler.getPLDMTypes(request, requestPayloadLength);
    // Need to support OEM type.
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());
    uint8_t* payload_ptr = responsePtr->payload;
    ASSERT_EQ(payload_ptr[0], 0);
    ASSERT_EQ(payload_ptr[1], 29); // 0b11101 see DSP0240 table11
    ASSERT_EQ(payload_ptr[2], 0);
}

TEST(GetPLDMCommands, testGoodRequest)
{
    // Need to support OEM type commands.
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_COMMANDS_REQ_BYTES>
        requestPayload{};
    auto request = reinterpret_cast<pldm_msg*>(requestPayload.data());
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);
    base::Handler handler;
    auto response = handler.getPLDMCommands(request, requestPayloadLength);
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());
    uint8_t* payload_ptr = responsePtr->payload;
    ASSERT_EQ(payload_ptr[0], 0);
    ASSERT_EQ(payload_ptr[1], 60); // 60 = 0b111100
    ASSERT_EQ(payload_ptr[2], 0);
}

TEST(GetPLDMCommands, testBadRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_COMMANDS_REQ_BYTES>
        requestPayload{};
    auto request = reinterpret_cast<pldm_msg*>(requestPayload.data());

    request->payload[0] = 0xFF;
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);
    base::Handler handler;
    auto response = handler.getPLDMCommands(request, requestPayloadLength);
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());
    uint8_t* payload_ptr = responsePtr->payload;
    ASSERT_EQ(payload_ptr[0], PLDM_ERROR_INVALID_PLDM_TYPE);
}
TEST(GetPLDMVersion, testGoodRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_REQ_BYTES>
        requestPayload{};
    auto request = reinterpret_cast<pldm_msg*>(requestPayload.data());
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    uint8_t pldmType = PLDM_BASE;
    uint32_t transferHandle = 0x0;
    uint8_t flag = PLDM_GET_FIRSTPART;
    uint8_t retFlag = PLDM_START_AND_END;
    ver32_t version = {0xF1, 0xF0, 0xF0, 0x00};

    auto rc =
        encode_get_version_req(0, transferHandle, flag, pldmType, request);

    ASSERT_EQ(0, rc);

    base::Handler handler;
    auto response = handler.getPLDMVersion(request, requestPayloadLength);
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());

    ASSERT_EQ(responsePtr->payload[0], 0);
    ASSERT_EQ(0, memcmp(responsePtr->payload + sizeof(responsePtr->payload[0]),
                        &transferHandle, sizeof(transferHandle)));
    ASSERT_EQ(0, memcmp(responsePtr->payload + sizeof(responsePtr->payload[0]) +
                            sizeof(transferHandle),
                        &retFlag, sizeof(flag)));
    ASSERT_EQ(0, memcmp(responsePtr->payload + sizeof(responsePtr->payload[0]) +
                            sizeof(transferHandle) + sizeof(flag),
                        &version, sizeof(version)));
}
TEST(GetPLDMVersion, testBadRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_REQ_BYTES>
        requestPayload{};
    auto request = reinterpret_cast<pldm_msg*>(requestPayload.data());
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    uint8_t pldmType = 7;
    uint32_t transferHandle = 0x0;
    uint8_t flag = PLDM_GET_FIRSTPART;

    auto rc =
        encode_get_version_req(0, transferHandle, flag, pldmType, request);

    ASSERT_EQ(0, rc);

    base::Handler handler;
    auto response = handler.getPLDMVersion(request, requestPayloadLength - 1);
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());

    ASSERT_EQ(responsePtr->payload[0], PLDM_ERROR_INVALID_LENGTH);

    request = reinterpret_cast<pldm_msg*>(requestPayload.data());
    requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    rc = encode_get_version_req(0, transferHandle, flag, pldmType, request);

    ASSERT_EQ(0, rc);

    response = handler.getPLDMVersion(request, requestPayloadLength);
    responsePtr = reinterpret_cast<pldm_msg*>(response.data());

    ASSERT_EQ(responsePtr->payload[0], PLDM_ERROR_INVALID_PLDM_TYPE);
}

TEST(GetTID, testGoodRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr)> requestPayload{};
    auto request = reinterpret_cast<pldm_msg*>(requestPayload.data());
    size_t requestPayloadLength = 0;

    base::Handler handler;
    auto response = handler.getTID(request, requestPayloadLength);

    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());
    uint8_t* payload = responsePtr->payload;

    ASSERT_EQ(payload[0], 0);
    ASSERT_EQ(payload[1], 1);
}
