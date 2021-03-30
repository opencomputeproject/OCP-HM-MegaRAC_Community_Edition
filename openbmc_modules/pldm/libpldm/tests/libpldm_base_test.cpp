#include <string.h>

#include <array>
#include <cstring>
#include <vector>

#include "libpldm/base.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::ElementsAreArray;

constexpr auto hdrSize = sizeof(pldm_msg_hdr);

TEST(PackPLDMMessage, BadPathTest)
{
    struct pldm_header_info hdr;
    struct pldm_header_info* hdr_ptr = NULL;
    pldm_msg_hdr msg{};

    // PLDM header information pointer is NULL
    auto rc = pack_pldm_header(hdr_ptr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // PLDM message pointer is NULL
    rc = pack_pldm_header(&hdr, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // PLDM header information pointer and PLDM message pointer is NULL
    rc = pack_pldm_header(hdr_ptr, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // RESERVED message type
    hdr.msg_type = PLDM_RESERVED;
    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Instance ID out of range
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 32;
    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // PLDM type out of range
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 31;
    hdr.pldm_type = 64;
    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_PLDM_TYPE);
}

TEST(PackPLDMMessage, RequestMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Message type is REQUEST and lower range of the field values
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 0;
    hdr.pldm_type = 0;
    hdr.command = 0;

    auto rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 1);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 0);
    EXPECT_EQ(msg.type, 0);
    EXPECT_EQ(msg.command, 0);

    // Message type is REQUEST and upper range of the field values
    hdr.instance = 31;
    hdr.pldm_type = 63;
    hdr.command = 255;

    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 1);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 31);
    EXPECT_EQ(msg.type, 63);
    EXPECT_EQ(msg.command, 255);

    // Message type is PLDM_ASYNC_REQUEST_NOTIFY
    hdr.msg_type = PLDM_ASYNC_REQUEST_NOTIFY;

    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 1);
    EXPECT_EQ(msg.datagram, 1);
    EXPECT_EQ(msg.instance_id, 31);
    EXPECT_EQ(msg.type, 63);
    EXPECT_EQ(msg.command, 255);
}

TEST(PackPLDMMessage, ResponseMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Message type is PLDM_RESPONSE and lower range of the field values
    hdr.msg_type = PLDM_RESPONSE;
    hdr.instance = 0;
    hdr.pldm_type = 0;
    hdr.command = 0;

    auto rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 0);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 0);
    EXPECT_EQ(msg.type, 0);
    EXPECT_EQ(msg.command, 0);

    // Message type is PLDM_RESPONSE and upper range of the field values
    hdr.instance = 31;
    hdr.pldm_type = 63;
    hdr.command = 255;

    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 0);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 31);
    EXPECT_EQ(msg.type, 63);
    EXPECT_EQ(msg.command, 255);
}

TEST(UnpackPLDMMessage, BadPathTest)
{
    struct pldm_header_info hdr;

    // PLDM message pointer is NULL
    auto rc = unpack_pldm_header(nullptr, &hdr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(UnpackPLDMMessage, RequestMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Unpack PLDM request message and lower range of field values
    msg.request = 1;
    auto rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_REQUEST);
    EXPECT_EQ(hdr.instance, 0);
    EXPECT_EQ(hdr.pldm_type, 0);
    EXPECT_EQ(hdr.command, 0);

    // Unpack PLDM async request message and lower range of field values
    msg.datagram = 1;
    rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_ASYNC_REQUEST_NOTIFY);

    // Unpack PLDM request message and upper range of field values
    msg.datagram = 0;
    msg.instance_id = 31;
    msg.type = 63;
    msg.command = 255;
    rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_REQUEST);
    EXPECT_EQ(hdr.instance, 31);
    EXPECT_EQ(hdr.pldm_type, 63);
    EXPECT_EQ(hdr.command, 255);
}

TEST(UnpackPLDMMessage, ResponseMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Unpack PLDM response message and lower range of field values
    auto rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_RESPONSE);
    EXPECT_EQ(hdr.instance, 0);
    EXPECT_EQ(hdr.pldm_type, 0);
    EXPECT_EQ(hdr.command, 0);

    // Unpack PLDM response message and upper range of field values
    msg.instance_id = 31;
    msg.type = 63;
    msg.command = 255;
    rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_RESPONSE);
    EXPECT_EQ(hdr.instance, 31);
    EXPECT_EQ(hdr.pldm_type, 63);
    EXPECT_EQ(hdr.command, 255);
}

TEST(GetPLDMCommands, testEncodeRequest)
{
    uint8_t pldmType = 0x05;
    ver32_t version{0xFF, 0xFF, 0xFF, 0xFF};
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_COMMANDS_REQ_BYTES>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_get_commands_req(0, pldmType, version, request);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(0, memcmp(request->payload, &pldmType, sizeof(pldmType)));
    EXPECT_EQ(0, memcmp(request->payload + sizeof(pldmType), &version,
                        sizeof(version)));
}

TEST(GetPLDMCommands, testDecodeRequest)
{
    uint8_t pldmType = 0x05;
    ver32_t version{0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t pldmTypeOut{};
    ver32_t versionOut{0xFF, 0xFF, 0xFF, 0xFF};
    std::array<uint8_t, hdrSize + PLDM_GET_COMMANDS_REQ_BYTES> requestMsg{};

    memcpy(requestMsg.data() + hdrSize, &pldmType, sizeof(pldmType));
    memcpy(requestMsg.data() + sizeof(pldmType) + hdrSize, &version,
           sizeof(version));

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = decode_get_commands_req(request, requestMsg.size() - hdrSize,
                                      &pldmTypeOut, &versionOut);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(pldmTypeOut, pldmType);
    EXPECT_EQ(0, memcmp(&versionOut, &version, sizeof(version)));
}

TEST(GetPLDMCommands, testEncodeResponse)
{
    uint8_t completionCode = 0;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_COMMANDS_RESP_BYTES>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    std::array<bitfield8_t, PLDM_MAX_CMDS_PER_TYPE / 8> commands{};
    commands[0].byte = 1;
    commands[1].byte = 2;
    commands[2].byte = 3;

    auto rc =
        encode_get_commands_resp(0, PLDM_SUCCESS, commands.data(), response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    uint8_t* payload_ptr = response->payload;
    EXPECT_EQ(completionCode, payload_ptr[0]);
    EXPECT_EQ(1, payload_ptr[sizeof(completionCode)]);
    EXPECT_EQ(2,
              payload_ptr[sizeof(completionCode) + sizeof(commands[0].byte)]);
    EXPECT_EQ(3, payload_ptr[sizeof(completionCode) + sizeof(commands[0].byte) +
                             sizeof(commands[1].byte)]);
}

TEST(GetPLDMTypes, testEncodeResponse)
{
    uint8_t completionCode = 0;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_TYPES_RESP_BYTES>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    std::array<bitfield8_t, PLDM_MAX_TYPES / 8> types{};
    types[0].byte = 1;
    types[1].byte = 2;
    types[2].byte = 3;

    auto rc = encode_get_types_resp(0, PLDM_SUCCESS, types.data(), response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    uint8_t* payload_ptr = response->payload;
    EXPECT_EQ(completionCode, payload_ptr[0]);
    EXPECT_EQ(1, payload_ptr[sizeof(completionCode)]);
    EXPECT_EQ(2, payload_ptr[sizeof(completionCode) + sizeof(types[0].byte)]);
    EXPECT_EQ(3, payload_ptr[sizeof(completionCode) + sizeof(types[0].byte) +
                             sizeof(types[1].byte)]);
}

TEST(GetPLDMTypes, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_TYPES_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_TYPES / 8> outTypes{};

    uint8_t completion_code;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_types_resp(response, responseMsg.size() - hdrSize,
                                    &completion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, PLDM_SUCCESS);
    EXPECT_EQ(responseMsg[1 + hdrSize], outTypes[0].byte);
    EXPECT_EQ(responseMsg[2 + hdrSize], outTypes[1].byte);
    EXPECT_EQ(responseMsg[3 + hdrSize], outTypes[2].byte);
}

TEST(GetPLDMTypes, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_TYPES_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_TYPES / 8> outTypes{};

    uint8_t retcompletion_code = 0;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_types_resp(response, responseMsg.size() - hdrSize - 1,
                                    &retcompletion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetPLDMCommands, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_COMMANDS_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_CMDS_PER_TYPE / 8> outTypes{};

    uint8_t completion_code;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_commands_resp(response, responseMsg.size() - hdrSize,
                                       &completion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, PLDM_SUCCESS);
    EXPECT_EQ(responseMsg[1 + hdrSize], outTypes[0].byte);
    EXPECT_EQ(responseMsg[2 + hdrSize], outTypes[1].byte);
    EXPECT_EQ(responseMsg[3 + hdrSize], outTypes[2].byte);
}

TEST(GetPLDMCommands, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_COMMANDS_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_CMDS_PER_TYPE / 8> outTypes{};

    uint8_t retcompletion_code = 0;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc =
        decode_get_commands_resp(response, responseMsg.size() - hdrSize - 1,
                                 &retcompletion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetPLDMVersion, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_REQ_BYTES>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    uint8_t pldmType = 0x03;
    uint32_t transferHandle = 0x0;
    uint8_t opFlag = 0x01;

    auto rc =
        encode_get_version_req(0, transferHandle, opFlag, pldmType, request);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(
        0, memcmp(request->payload, &transferHandle, sizeof(transferHandle)));
    EXPECT_EQ(0, memcmp(request->payload + sizeof(transferHandle), &opFlag,
                        sizeof(opFlag)));
    EXPECT_EQ(0,
              memcmp(request->payload + sizeof(transferHandle) + sizeof(opFlag),
                     &pldmType, sizeof(pldmType)));
}

TEST(GetPLDMVersion, testBadEncodeRequest)
{
    uint8_t pldmType = 0x03;
    uint32_t transferHandle = 0x0;
    uint8_t opFlag = 0x01;

    auto rc =
        encode_get_version_req(0, transferHandle, opFlag, pldmType, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetPLDMVersion, testEncodeResponse)
{
    uint8_t completionCode = 0;
    uint32_t transferHandle = 0;
    uint8_t flag = PLDM_START_AND_END;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_RESP_BYTES>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    ver32_t version = {0xFF, 0xFF, 0xFF, 0xFF};

    auto rc = encode_get_version_resp(0, PLDM_SUCCESS, 0, PLDM_START_AND_END,
                                      &version, sizeof(ver32_t), response);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, response->payload[0]);
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]),
                        &transferHandle, sizeof(transferHandle)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(transferHandle),
                        &flag, sizeof(flag)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(transferHandle) + sizeof(flag),
                        &version, sizeof(version)));
}

TEST(GetPLDMVersion, testDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_VERSION_REQ_BYTES> requestMsg{};
    uint32_t transferHandle = 0x0;
    uint32_t retTransferHandle = 0x0;
    uint8_t flag = PLDM_GET_FIRSTPART;
    uint8_t retFlag = PLDM_GET_FIRSTPART;
    uint8_t pldmType = PLDM_BASE;
    uint8_t retType = PLDM_BASE;

    memcpy(requestMsg.data() + hdrSize, &transferHandle,
           sizeof(transferHandle));
    memcpy(requestMsg.data() + sizeof(transferHandle) + hdrSize, &flag,
           sizeof(flag));
    memcpy(requestMsg.data() + sizeof(transferHandle) + sizeof(flag) + hdrSize,
           &pldmType, sizeof(pldmType));

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = decode_get_version_req(request, requestMsg.size() - hdrSize,
                                     &retTransferHandle, &retFlag, &retType);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(transferHandle, retTransferHandle);
    EXPECT_EQ(flag, retFlag);
    EXPECT_EQ(pldmType, retType);
}

TEST(GetPLDMVersion, testDecodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_RESP_BYTES>
        responseMsg{};
    uint32_t transferHandle = 0x0;
    uint32_t retTransferHandle = 0x0;
    uint8_t flag = PLDM_START_AND_END;
    uint8_t retFlag = PLDM_START_AND_END;
    uint8_t completionCode = 0;
    ver32_t version = {0xFF, 0xFF, 0xFF, 0xFF};
    ver32_t versionOut;
    uint8_t completion_code;

    memcpy(responseMsg.data() + sizeof(completionCode) + hdrSize,
           &transferHandle, sizeof(transferHandle));
    memcpy(responseMsg.data() + sizeof(completionCode) +
               sizeof(transferHandle) + hdrSize,
           &flag, sizeof(flag));
    memcpy(responseMsg.data() + sizeof(completionCode) +
               sizeof(transferHandle) + sizeof(flag) + hdrSize,
           &version, sizeof(version));

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_version_resp(response, responseMsg.size() - hdrSize,
                                      &completion_code, &retTransferHandle,
                                      &retFlag, &versionOut);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(transferHandle, retTransferHandle);
    EXPECT_EQ(flag, retFlag);

    EXPECT_EQ(versionOut.major, version.major);
    EXPECT_EQ(versionOut.minor, version.minor);
    EXPECT_EQ(versionOut.update, version.update);
    EXPECT_EQ(versionOut.alpha, version.alpha);
}

TEST(GetTID, testEncodeRequest)
{
    pldm_msg request{};

    auto rc = encode_get_tid_req(0, &request);
    ASSERT_EQ(rc, PLDM_SUCCESS);
}

TEST(GetTID, testEncodeResponse)
{
    uint8_t completionCode = 0;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_TID_RESP_BYTES>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    uint8_t tid = 1;

    auto rc = encode_get_tid_resp(0, PLDM_SUCCESS, tid, response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    uint8_t* payload = response->payload;
    EXPECT_EQ(completionCode, payload[0]);
    EXPECT_EQ(1, payload[sizeof(completionCode)]);
}

TEST(GetTID, testDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_TID_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;

    uint8_t tid;
    uint8_t completion_code;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_tid_resp(response, responseMsg.size() - hdrSize,
                                  &completion_code, &tid);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, PLDM_SUCCESS);
    EXPECT_EQ(tid, 1);
}

TEST(CcOnlyResponse, testEncode)
{
    struct pldm_msg responseMsg;

    auto rc =
        encode_cc_only_resp(0 /*instance id*/, 1 /*pldm type*/, 2 /*command*/,
                            3 /*complection code*/, &responseMsg);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    auto p = reinterpret_cast<uint8_t*>(&responseMsg);
    EXPECT_THAT(std::vector<uint8_t>(p, p + sizeof(responseMsg)),
                ElementsAreArray({0, 1, 2, 3}));

    rc = encode_cc_only_resp(PLDM_INSTANCE_MAX + 1, 1, 2, 3, &responseMsg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_cc_only_resp(0, 1, 2, 3, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}
