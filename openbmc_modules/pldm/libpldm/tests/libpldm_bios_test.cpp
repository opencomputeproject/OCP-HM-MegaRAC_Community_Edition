#include <string.h>

#include <array>

#include "libpldm/base.h"
#include "libpldm/bios.h"
#include "libpldm/utils.h"

#include <gtest/gtest.h>

constexpr auto hdrSize = sizeof(pldm_msg_hdr);

TEST(GetDateTime, testEncodeRequest)
{
    pldm_msg request{};

    auto rc = encode_get_date_time_req(0, &request);
    EXPECT_EQ(rc, PLDM_SUCCESS);
}

TEST(GetDateTime, testEncodeResponse)
{
    uint8_t completionCode = 0;
    uint8_t seconds = 50;
    uint8_t minutes = 20;
    uint8_t hours = 5;
    uint8_t day = 23;
    uint8_t month = 11;
    uint16_t year = 2019;

    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_DATE_TIME_RESP_BYTES>
        responseMsg{};

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_get_date_time_resp(0, PLDM_SUCCESS, seconds, minutes,
                                        hours, day, month, year, response);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, response->payload[0]);

    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]),
                        &seconds, sizeof(seconds)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(seconds),
                        &minutes, sizeof(minutes)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(seconds) + sizeof(minutes),
                        &hours, sizeof(hours)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(seconds) + sizeof(minutes) + sizeof(hours),
                        &day, sizeof(day)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(seconds) + sizeof(minutes) + sizeof(hours) +
                            sizeof(day),
                        &month, sizeof(month)));
    uint16_t yearLe = htole16(year);
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(seconds) + sizeof(minutes) + sizeof(hours) +
                            sizeof(day) + sizeof(month),
                        &yearLe, sizeof(yearLe)));
}

TEST(GetDateTime, testDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_DATE_TIME_RESP_BYTES> responseMsg{};

    uint8_t completionCode = 0;

    uint8_t seconds = 55;
    uint8_t minutes = 2;
    uint8_t hours = 8;
    uint8_t day = 9;
    uint8_t month = 7;
    uint16_t year = 2020;
    uint16_t yearLe = htole16(year);

    uint8_t retSeconds = 0;
    uint8_t retMinutes = 0;
    uint8_t retHours = 0;
    uint8_t retDay = 0;
    uint8_t retMonth = 0;
    uint16_t retYear = 0;

    memcpy(responseMsg.data() + sizeof(completionCode) + hdrSize, &seconds,
           sizeof(seconds));
    memcpy(responseMsg.data() + sizeof(completionCode) + sizeof(seconds) +
               hdrSize,
           &minutes, sizeof(minutes));
    memcpy(responseMsg.data() + sizeof(completionCode) + sizeof(seconds) +
               sizeof(minutes) + hdrSize,
           &hours, sizeof(hours));
    memcpy(responseMsg.data() + sizeof(completionCode) + sizeof(seconds) +
               sizeof(minutes) + sizeof(hours) + hdrSize,
           &day, sizeof(day));
    memcpy(responseMsg.data() + sizeof(completionCode) + sizeof(seconds) +
               sizeof(minutes) + sizeof(hours) + sizeof(day) + hdrSize,
           &month, sizeof(month));
    memcpy(responseMsg.data() + sizeof(completionCode) + sizeof(seconds) +
               sizeof(minutes) + sizeof(hours) + sizeof(day) + sizeof(month) +
               hdrSize,
           &yearLe, sizeof(yearLe));

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_date_time_resp(
        response, responseMsg.size() - hdrSize, &completionCode, &retSeconds,
        &retMinutes, &retHours, &retDay, &retMonth, &retYear);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(seconds, retSeconds);
    EXPECT_EQ(minutes, retMinutes);
    EXPECT_EQ(hours, retHours);
    EXPECT_EQ(day, retDay);
    EXPECT_EQ(month, retMonth);
    EXPECT_EQ(year, retYear);
}

TEST(SetDateTime, testGoodEncodeResponse)
{
    uint8_t instanceId = 0;
    uint8_t completionCode = PLDM_SUCCESS;

    std::array<uint8_t, hdrSize + sizeof(struct pldm_only_cc_resp)>
        responseMsg{};
    struct pldm_msg* response =
        reinterpret_cast<struct pldm_msg*>(responseMsg.data());

    auto rc = encode_set_date_time_resp(instanceId, completionCode, response,
                                        responseMsg.size() - hdrSize);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_only_cc_resp* resp =
        reinterpret_cast<struct pldm_only_cc_resp*>(response->payload);
    EXPECT_EQ(completionCode, resp->completion_code);
}

TEST(SetDateTime, testBadEncodeResponse)
{

    uint8_t instanceId = 10;
    uint8_t completionCode = PLDM_SUCCESS;
    std::array<uint8_t, hdrSize + sizeof(struct pldm_only_cc_resp)>
        responseMsg{};
    struct pldm_msg* response =
        reinterpret_cast<struct pldm_msg*>(responseMsg.data());
    auto rc = encode_set_date_time_resp(instanceId, completionCode, nullptr,
                                        responseMsg.size() - hdrSize);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
    rc = encode_set_date_time_resp(instanceId, completionCode, response,
                                   responseMsg.size() - hdrSize - 1);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(SetDateTime, testGoodDecodeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;
    std::array<uint8_t, hdrSize + sizeof(struct pldm_only_cc_resp)>
        responseMsg{};
    struct pldm_msg* response =
        reinterpret_cast<struct pldm_msg*>(responseMsg.data());
    struct pldm_only_cc_resp* resp =
        reinterpret_cast<struct pldm_only_cc_resp*>(response->payload);

    resp->completion_code = completionCode;

    uint8_t retCompletionCode;
    auto rc = decode_set_date_time_resp(response, responseMsg.size() - hdrSize,
                                        &retCompletionCode);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, retCompletionCode);
}

TEST(SetDateTime, testBadDecodeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;

    std::array<uint8_t, hdrSize + sizeof(struct pldm_only_cc_resp)>
        responseMsg{};
    struct pldm_msg* response =
        reinterpret_cast<struct pldm_msg*>(responseMsg.data());
    auto rc = decode_set_date_time_resp(nullptr, responseMsg.size() - hdrSize,
                                        &completionCode);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_set_date_time_resp(response, responseMsg.size() - hdrSize - 1,
                                   &completionCode);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(SetDateTime, testGoodEncodeRequset)
{
    uint8_t instanceId = 0;
    uint8_t seconds = 50;
    uint8_t minutes = 20;
    uint8_t hours = 10;
    uint8_t day = 11;
    uint8_t month = 11;
    uint16_t year = 2019;

    std::array<uint8_t, hdrSize + sizeof(struct pldm_set_date_time_req)>
        requestMsg{};

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = encode_set_date_time_req(instanceId, seconds, minutes, hours, day,
                                       month, year, request,
                                       requestMsg.size() - hdrSize);

    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_set_date_time_req* req =
        reinterpret_cast<struct pldm_set_date_time_req*>(request->payload);
    EXPECT_EQ(seconds, bcd2dec8(req->seconds));
    EXPECT_EQ(minutes, bcd2dec8(req->minutes));
    EXPECT_EQ(hours, bcd2dec8(req->hours));
    EXPECT_EQ(day, bcd2dec8(req->day));
    EXPECT_EQ(month, bcd2dec8(req->month));
    EXPECT_EQ(year, bcd2dec16(le16toh(req->year)));
}

TEST(SetDateTime, testBadEncodeRequset)
{
    uint8_t instanceId = 0;

    uint8_t seconds = 50;
    uint8_t minutes = 20;
    uint8_t hours = 10;
    uint8_t day = 13;
    uint8_t month = 11;
    uint16_t year = 2019;

    uint8_t erday = 43;

    std::array<uint8_t, hdrSize + sizeof(struct pldm_set_date_time_req)>
        requestMsg{};

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_set_date_time_req(instanceId, seconds, minutes, hours, day,
                                       month, year, nullptr,
                                       requestMsg.size() - hdrSize);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_set_date_time_req(instanceId, seconds, minutes, hours, erday,
                                  month, year, request,
                                  requestMsg.size() - hdrSize);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_set_date_time_req(instanceId, seconds, minutes, hours, day,
                                  month, year, request,
                                  requestMsg.size() - hdrSize - 4);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(SetDateTime, testGoodDecodeRequest)
{
    std::array<uint8_t, hdrSize + sizeof(struct pldm_set_date_time_req)>
        requestMsg{};
    uint8_t seconds = 0x50;
    uint8_t minutes = 0x20;
    uint8_t hours = 0x10;
    uint8_t day = 0x11;
    uint8_t month = 0x11;
    uint16_t year = 0x2019;

    auto request = reinterpret_cast<struct pldm_msg*>(requestMsg.data());
    struct pldm_set_date_time_req* req =
        reinterpret_cast<struct pldm_set_date_time_req*>(request->payload);
    req->seconds = seconds;
    req->minutes = minutes;
    req->hours = hours;
    req->day = day;
    req->month = month;
    req->year = htole16(year);

    uint8_t retseconds;
    uint8_t retminutes;
    uint8_t rethours;
    uint8_t retday;
    uint8_t retmonth;
    uint16_t retyear;

    auto rc = decode_set_date_time_req(request, requestMsg.size() - hdrSize,
                                       &retseconds, &retminutes, &rethours,
                                       &retday, &retmonth, &retyear);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retseconds, 50);
    EXPECT_EQ(retminutes, 20);
    EXPECT_EQ(rethours, 10);
    EXPECT_EQ(retday, 11);
    EXPECT_EQ(retmonth, 11);
    EXPECT_EQ(retyear, 2019);
}

TEST(SetDateTime, testBadDecodeRequest)
{
    uint8_t seconds = 0x50;
    uint8_t minutes = 0x20;
    uint8_t hours = 0x10;
    uint8_t day = 0x11;
    uint8_t month = 0x11;
    uint16_t year = htole16(0x2019);

    std::array<uint8_t, hdrSize + sizeof(struct pldm_set_date_time_req)>
        requestMsg{};

    auto request = reinterpret_cast<struct pldm_msg*>(requestMsg.data());

    decode_set_date_time_req(request, requestMsg.size() - hdrSize, &seconds,
                             &minutes, &hours, &day, &month, &year);

    auto rc =
        decode_set_date_time_req(nullptr, requestMsg.size() - hdrSize, &seconds,
                                 &minutes, &hours, &day, &month, &year);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_set_date_time_req(request, requestMsg.size() - hdrSize, nullptr,
                                  nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_set_date_time_req(request, requestMsg.size() - hdrSize - 4,
                                  &seconds, &minutes, &hours, &day, &month,
                                  &year);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetBIOSTable, testGoodEncodeResponse)
{
    std::array<uint8_t,
               sizeof(pldm_msg_hdr) + PLDM_GET_BIOS_TABLE_MIN_RESP_BYTES + 4>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    uint8_t completionCode = PLDM_SUCCESS;
    uint32_t nextTransferHandle = 32;
    uint8_t transferFlag = PLDM_START_AND_END;
    std::array<uint8_t, 4> tableData{1, 2, 3, 4};

    auto rc = encode_get_bios_table_resp(
        0, PLDM_SUCCESS, nextTransferHandle, transferFlag, tableData.data(),
        sizeof(pldm_msg_hdr) + PLDM_GET_BIOS_TABLE_MIN_RESP_BYTES + 4,
        response);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_get_bios_table_resp* resp =
        reinterpret_cast<struct pldm_get_bios_table_resp*>(response->payload);

    EXPECT_EQ(completionCode, resp->completion_code);
    EXPECT_EQ(nextTransferHandle, le32toh(resp->next_transfer_handle));
    EXPECT_EQ(transferFlag, resp->transfer_flag);
    EXPECT_EQ(0, memcmp(tableData.data(), resp->table_data, tableData.size()));
}

TEST(GetBIOSTable, testBadEncodeResponse)
{
    uint32_t nextTransferHandle = 32;
    uint8_t transferFlag = PLDM_START_AND_END;
    std::array<uint8_t, 4> tableData{1, 2, 3, 4};

    auto rc = encode_get_bios_table_resp(
        0, PLDM_SUCCESS, nextTransferHandle, transferFlag, tableData.data(),
        sizeof(pldm_msg_hdr) + PLDM_GET_BIOS_TABLE_MIN_RESP_BYTES + 4, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetBIOSTable, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_BIOS_TABLE_REQ_BYTES>
        requestMsg{};
    uint32_t transferHandle = 0x0;
    uint8_t transferOpFlag = 0x01;
    uint8_t tableType = PLDM_BIOS_ATTR_TABLE;

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = encode_get_bios_table_req(0, transferHandle, transferOpFlag,
                                        tableType, request);

    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_get_bios_table_req* req =
        reinterpret_cast<struct pldm_get_bios_table_req*>(request->payload);
    EXPECT_EQ(transferHandle, le32toh(req->transfer_handle));
    EXPECT_EQ(transferOpFlag, req->transfer_op_flag);
    EXPECT_EQ(tableType, req->table_type);
}

TEST(GetBIOSTable, testBadEncodeRequest)
{
    uint32_t transferHandle = 0x0;
    uint8_t transferOpFlag = 0x01;
    uint8_t tableType = PLDM_BIOS_ATTR_TABLE;

    auto rc = encode_get_bios_table_req(0, transferHandle, transferOpFlag,
                                        tableType, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetBIOSTable, testGoodDecodeRequest)
{
    const auto hdr_size = sizeof(pldm_msg_hdr);
    std::array<uint8_t, hdr_size + PLDM_GET_BIOS_TABLE_REQ_BYTES> requestMsg{};
    uint32_t transferHandle = 31;
    uint8_t transferOpFlag = PLDM_GET_FIRSTPART;
    uint8_t tableType = PLDM_BIOS_ATTR_TABLE;
    uint32_t retTransferHandle = 0;
    uint8_t retTransferOpFlag = 0;
    uint8_t retTableType = 0;

    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_get_bios_table_req* request =
        reinterpret_cast<struct pldm_get_bios_table_req*>(req->payload);

    request->transfer_handle = htole32(transferHandle);
    request->transfer_op_flag = transferOpFlag;
    request->table_type = tableType;

    auto rc = decode_get_bios_table_req(req, requestMsg.size() - hdr_size,
                                        &retTransferHandle, &retTransferOpFlag,
                                        &retTableType);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(transferHandle, retTransferHandle);
    EXPECT_EQ(transferOpFlag, retTransferOpFlag);
    EXPECT_EQ(tableType, retTableType);
}
TEST(GetBIOSTable, testBadDecodeRequest)
{
    const auto hdr_size = sizeof(pldm_msg_hdr);
    std::array<uint8_t, hdr_size + PLDM_GET_BIOS_TABLE_REQ_BYTES> requestMsg{};
    uint32_t transferHandle = 31;
    uint8_t transferOpFlag = PLDM_GET_FIRSTPART;
    uint8_t tableType = PLDM_BIOS_ATTR_TABLE;
    uint32_t retTransferHandle = 0;
    uint8_t retTransferOpFlag = 0;
    uint8_t retTableType = 0;

    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_get_bios_table_req* request =
        reinterpret_cast<struct pldm_get_bios_table_req*>(req->payload);

    request->transfer_handle = htole32(transferHandle);
    request->transfer_op_flag = transferOpFlag;
    request->table_type = tableType;

    auto rc = decode_get_bios_table_req(req, requestMsg.size() - hdr_size,
                                        &retTransferHandle, &retTransferOpFlag,
                                        &retTableType);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(transferHandle, retTransferHandle);
    EXPECT_EQ(transferOpFlag, retTransferOpFlag);
    EXPECT_EQ(tableType, retTableType);
}
/*
TEST(GetBIOSTable, testBadDecodeRequest)
{
    const auto hdr_size = sizeof(pldm_msg_hdr);
    std::array<uint8_t, hdr_size + PLDM_GET_BIOS_TABLE_REQ_BYTES> requestMsg{};
    uint32_t transferHandle = 31;
    uint8_t transferOpFlag = PLDM_GET_FIRSTPART;
    uint8_t tableType = PLDM_BIOS_ATTR_TABLE;
    uint32_t retTransferHandle = 0;
    uint8_t retTransferOpFlag = 0;
    uint8_t retTableType = 0;

    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_get_bios_table_req* request =
        reinterpret_cast<struct pldm_get_bios_table_req*>(req->payload);

    request->transfer_handle = htole32(transferHandle);
    request->transfer_op_flag = transferOpFlag;
    request->table_type = tableType;

    auto rc = decode_get_bios_table_req(req, requestMsg.size() - hdr_size - 3,
                                        &retTransferHandle, &retTransferOpFlag,
                                        &retTableType);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}*/

TEST(GetBIOSAttributeCurrentValueByHandle, testGoodDecodeRequest)
{
    uint32_t transferHandle = 45;
    uint8_t transferOpFlag = PLDM_GET_FIRSTPART;
    uint16_t attributehandle = 10;
    uint32_t retTransferHandle = 0;
    uint8_t retTransferOpFlag = 0;
    uint16_t retattributehandle = 0;
    std::array<uint8_t, hdrSize + sizeof(transferHandle) +
                            sizeof(transferOpFlag) + sizeof(attributehandle)>
        requestMsg{};

    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_get_bios_attribute_current_value_by_handle_req* request =
        reinterpret_cast<
            struct pldm_get_bios_attribute_current_value_by_handle_req*>(
            req->payload);

    request->transfer_handle = htole32(transferHandle);
    request->transfer_op_flag = transferOpFlag;
    request->attribute_handle = htole16(attributehandle);

    auto rc = decode_get_bios_attribute_current_value_by_handle_req(
        req, requestMsg.size() - hdrSize, &retTransferHandle,
        &retTransferOpFlag, &retattributehandle);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(transferHandle, retTransferHandle);
    EXPECT_EQ(transferOpFlag, retTransferOpFlag);
    EXPECT_EQ(attributehandle, retattributehandle);
}

TEST(GetBIOSAttributeCurrentValueByHandle, testBadDecodeRequest)
{

    uint32_t transferHandle = 0;
    uint8_t transferOpFlag = PLDM_GET_FIRSTPART;
    uint16_t attribute_handle = 0;
    uint32_t retTransferHandle = 0;
    uint8_t retTransferOpFlag = 0;
    uint16_t retattribute_handle = 0;
    std::array<uint8_t, hdrSize + sizeof(transferHandle) +
                            sizeof(transferOpFlag) + sizeof(attribute_handle)>
        requestMsg{};

    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_get_bios_attribute_current_value_by_handle_req* request =
        reinterpret_cast<
            struct pldm_get_bios_attribute_current_value_by_handle_req*>(
            req->payload);

    request->transfer_handle = htole32(transferHandle);
    request->transfer_op_flag = transferOpFlag;
    request->attribute_handle = attribute_handle;

    auto rc = decode_get_bios_attribute_current_value_by_handle_req(
        NULL, requestMsg.size() - hdrSize, &retTransferHandle,
        &retTransferOpFlag, &retattribute_handle);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    transferHandle = 31;
    request->transfer_handle = htole32(transferHandle);

    rc = decode_get_bios_attribute_current_value_by_handle_req(
        req, 0, &retTransferHandle, &retTransferOpFlag, &retattribute_handle);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetBIOSAttributeCurrentValueByHandle, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) +
                            PLDM_GET_BIOS_ATTR_CURR_VAL_BY_HANDLE_REQ_BYTES>
        requestMsg{};
    uint32_t transferHandle = 45;
    uint8_t transferOpFlag = PLDM_GET_FIRSTPART;
    uint8_t attributeHandle = 10;

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = encode_get_bios_attribute_current_value_by_handle_req(
        0, transferHandle, transferOpFlag, attributeHandle, request);

    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_get_bios_attribute_current_value_by_handle_req* req =
        reinterpret_cast<
            struct pldm_get_bios_attribute_current_value_by_handle_req*>(
            request->payload);
    EXPECT_EQ(transferHandle, le32toh(req->transfer_handle));
    EXPECT_EQ(transferOpFlag, req->transfer_op_flag);
    EXPECT_EQ(attributeHandle, le16toh(req->attribute_handle));
}

TEST(GetBIOSAttributeCurrentValueByHandle, testBadEncodeRequest)
{
    uint32_t transferHandle = 0;
    uint8_t transferOpFlag = PLDM_GET_FIRSTPART;
    uint8_t attributeHandle = 0;

    auto rc = encode_get_bios_attribute_current_value_by_handle_req(
        0, transferHandle, transferOpFlag, attributeHandle, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetBIOSAttributeCurrentValueByHandle, testGoodEncodeResponse)
{

    uint8_t instanceId = 10;
    uint8_t completionCode = PLDM_SUCCESS;
    uint32_t nextTransferHandle = 32;
    uint8_t transferFlag = PLDM_START_AND_END;
    uint8_t attributeData = 44;
    std::array<uint8_t,
               hdrSize +
                   sizeof(pldm_get_bios_attribute_current_value_by_handle_resp)>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_get_bios_current_value_by_handle_resp(
        instanceId, completionCode, nextTransferHandle, transferFlag,
        &attributeData, sizeof(attributeData), response);

    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_get_bios_attribute_current_value_by_handle_resp* resp =
        reinterpret_cast<
            struct pldm_get_bios_attribute_current_value_by_handle_resp*>(
            response->payload);

    EXPECT_EQ(completionCode, resp->completion_code);
    EXPECT_EQ(nextTransferHandle, le32toh(resp->next_transfer_handle));
    EXPECT_EQ(transferFlag, resp->transfer_flag);
    EXPECT_EQ(
        0, memcmp(&attributeData, resp->attribute_data, sizeof(attributeData)));
}

TEST(GetBIOSAttributeCurrentValueByHandle, testBadEncodeResponse)
{
    uint32_t nextTransferHandle = 32;
    uint8_t transferFlag = PLDM_START_AND_END;
    uint8_t attributeData = 44;

    auto rc = encode_get_bios_current_value_by_handle_resp(
        0, PLDM_SUCCESS, nextTransferHandle, transferFlag, &attributeData,
        sizeof(attributeData), nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t,
               hdrSize +
                   sizeof(pldm_get_bios_attribute_current_value_by_handle_resp)>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    rc = encode_get_bios_current_value_by_handle_resp(
        0, PLDM_SUCCESS, nextTransferHandle, transferFlag, nullptr,
        sizeof(attributeData), response);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(SetBiosAttributeCurrentValue, testGoodEncodeRequest)
{
    uint8_t instanceId = 10;
    uint32_t transferHandle = 32;
    uint8_t transferFlag = PLDM_START_AND_END;
    uint32_t attributeData = 44;
    std::array<uint8_t, hdrSize + PLDM_SET_BIOS_ATTR_CURR_VAL_MIN_REQ_BYTES +
                            sizeof(attributeData)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = encode_set_bios_attribute_current_value_req(
        instanceId, transferHandle, transferFlag,
        reinterpret_cast<uint8_t*>(&attributeData), sizeof(attributeData),
        request, requestMsg.size() - hdrSize);

    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_set_bios_attribute_current_value_req* req =
        reinterpret_cast<struct pldm_set_bios_attribute_current_value_req*>(
            request->payload);
    EXPECT_EQ(htole32(transferHandle), req->transfer_handle);
    EXPECT_EQ(transferFlag, req->transfer_flag);
    EXPECT_EQ(
        0, memcmp(&attributeData, req->attribute_data, sizeof(attributeData)));
}

TEST(SetBiosAttributeCurrentValue, testBadEncodeRequest)
{
    uint8_t instanceId = 10;
    uint32_t transferHandle = 32;
    uint8_t transferFlag = PLDM_START_AND_END;
    uint32_t attributeData = 44;
    std::array<uint8_t, hdrSize + PLDM_SET_BIOS_ATTR_CURR_VAL_MIN_REQ_BYTES>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_set_bios_attribute_current_value_req(
        instanceId, transferHandle, transferFlag, nullptr, 0, nullptr, 0);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
    rc = encode_set_bios_attribute_current_value_req(
        instanceId, transferHandle, transferFlag,
        reinterpret_cast<uint8_t*>(&attributeData), sizeof(attributeData),
        request, requestMsg.size() - hdrSize);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(SetBiosAttributeCurrentValue, testGoodDecodeRequest)
{
    uint32_t transferHandle = 32;
    uint8_t transferFlag = PLDM_START_AND_END;
    uint32_t attributeData = 44;

    std::array<uint8_t, hdrSize + PLDM_SET_BIOS_ATTR_CURR_VAL_MIN_REQ_BYTES +
                            sizeof(attributeData)>
        requestMsg{};
    auto request = reinterpret_cast<struct pldm_msg*>(requestMsg.data());
    struct pldm_set_bios_attribute_current_value_req* req =
        reinterpret_cast<struct pldm_set_bios_attribute_current_value_req*>(
            request->payload);
    req->transfer_handle = htole32(transferHandle);
    req->transfer_flag = transferFlag;
    memcpy(req->attribute_data, &attributeData, sizeof(attributeData));

    uint32_t retTransferHandle;
    uint8_t retTransferFlag;
    struct variable_field attribute;
    auto rc = decode_set_bios_attribute_current_value_req(
        request, requestMsg.size() - hdrSize, &retTransferHandle,
        &retTransferFlag, &attribute);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retTransferHandle, transferHandle);
    EXPECT_EQ(retTransferFlag, transferFlag);
    EXPECT_EQ(attribute.length, sizeof(attributeData));
    EXPECT_EQ(0, memcmp(attribute.ptr, &attributeData, sizeof(attributeData)));
}

TEST(SetBiosAttributeCurrentValue, testBadDecodeRequest)
{
    uint32_t transferHandle = 32;
    uint8_t transferFlag = PLDM_START_AND_END;
    struct variable_field attribute;
    std::array<uint8_t, hdrSize + PLDM_SET_BIOS_ATTR_CURR_VAL_MIN_REQ_BYTES - 1>
        requestMsg{};
    auto request = reinterpret_cast<struct pldm_msg*>(requestMsg.data());

    auto rc = decode_set_bios_attribute_current_value_req(
        nullptr, 0, &transferHandle, &transferFlag, &attribute);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
    rc = decode_set_bios_attribute_current_value_req(
        request, requestMsg.size() - hdrSize, &transferHandle, &transferFlag,
        &attribute);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(SetBiosAttributeCurrentValue, testGoodEncodeResponse)
{
    uint8_t instanceId = 10;
    uint32_t nextTransferHandle = 32;
    uint8_t completionCode = PLDM_SUCCESS;

    std::array<uint8_t, hdrSize + PLDM_SET_BIOS_ATTR_CURR_VAL_RESP_BYTES>
        responseMsg{};
    struct pldm_msg* response =
        reinterpret_cast<struct pldm_msg*>(responseMsg.data());
    auto rc = encode_set_bios_attribute_current_value_resp(
        instanceId, completionCode, nextTransferHandle, response);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_set_bios_attribute_current_value_resp* resp =
        reinterpret_cast<struct pldm_set_bios_attribute_current_value_resp*>(
            response->payload);
    EXPECT_EQ(completionCode, resp->completion_code);
    EXPECT_EQ(htole32(nextTransferHandle), resp->next_transfer_handle);
}

TEST(SetBiosAttributeCurrentValue, testBadEncodeResponse)
{
    uint8_t instanceId = 10;
    uint32_t nextTransferHandle = 32;
    uint8_t completionCode = PLDM_SUCCESS;
    auto rc = encode_set_bios_attribute_current_value_resp(
        instanceId, completionCode, nextTransferHandle, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}
TEST(SetBiosAttributeCurrentValue, testGoodDecodeResponse)
{
    uint32_t nextTransferHandle = 32;
    uint8_t completionCode = PLDM_SUCCESS;
    std::array<uint8_t, hdrSize + PLDM_SET_BIOS_ATTR_CURR_VAL_RESP_BYTES>
        responseMsg{};
    struct pldm_msg* response =
        reinterpret_cast<struct pldm_msg*>(responseMsg.data());
    struct pldm_set_bios_attribute_current_value_resp* resp =
        reinterpret_cast<struct pldm_set_bios_attribute_current_value_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->next_transfer_handle = htole32(nextTransferHandle);

    uint8_t retCompletionCode;
    uint32_t retNextTransferHandle;
    auto rc = decode_set_bios_attribute_current_value_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retNextTransferHandle);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, retCompletionCode);
    EXPECT_EQ(nextTransferHandle, retNextTransferHandle);
}

TEST(SetBiosAttributeCurrentValue, testBadDecodeResponse)
{
    uint32_t nextTransferHandle = 32;
    uint8_t completionCode = PLDM_SUCCESS;

    std::array<uint8_t, hdrSize + PLDM_SET_BIOS_ATTR_CURR_VAL_RESP_BYTES>
        responseMsg{};
    struct pldm_msg* response =
        reinterpret_cast<struct pldm_msg*>(responseMsg.data());
    struct pldm_set_bios_attribute_current_value_resp* resp =
        reinterpret_cast<struct pldm_set_bios_attribute_current_value_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->next_transfer_handle = htole32(nextTransferHandle);

    auto rc = decode_set_bios_attribute_current_value_resp(
        nullptr, 0, &completionCode, &nextTransferHandle);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_set_bios_attribute_current_value_resp(
        response, responseMsg.size() - hdrSize - 1, &completionCode,
        &nextTransferHandle);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetBIOSTable, testDecodeResponse)
{
    uint32_t nextTransferHandle = 32;
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transfer_flag = PLDM_START_AND_END;

    std::array<uint8_t, hdrSize + PLDM_GET_BIOS_TABLE_MIN_RESP_BYTES>
        responseMsg{};
    struct pldm_msg* response =
        reinterpret_cast<struct pldm_msg*>(responseMsg.data());

    struct pldm_get_bios_table_resp* resp =
        reinterpret_cast<struct pldm_get_bios_table_resp*>(response->payload);

    resp->completion_code = completionCode;
    resp->next_transfer_handle = htole32(nextTransferHandle);
    resp->transfer_flag = transfer_flag;
    size_t biosTableOffset = sizeof(completionCode) +
                             sizeof(nextTransferHandle) + sizeof(transfer_flag);

    uint8_t retCompletionCode;
    uint32_t retNextTransferHandle;
    uint8_t retransfer_flag;
    size_t rebiosTableOffset = 0;
    auto rc = decode_get_bios_table_resp(
        response, responseMsg.size(), &retCompletionCode,
        &retNextTransferHandle, &retransfer_flag, &rebiosTableOffset);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(completionCode, retCompletionCode);
    ASSERT_EQ(nextTransferHandle, retNextTransferHandle);
    ASSERT_EQ(transfer_flag, retransfer_flag);
    ASSERT_EQ(biosTableOffset, rebiosTableOffset);
}

TEST(GetBIOSAttributeCurrentValueByHandle, testDecodeResponse)
{
    uint32_t nextTransferHandle = 32;
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transfer_flag = PLDM_START_AND_END;
    uint32_t attributeData = 44;

    std::array<uint8_t,
               hdrSize + PLDM_GET_BIOS_ATTR_CURR_VAL_BY_HANDLE_MIN_RESP_BYTES +
                   sizeof(attributeData)>
        responseMsg{};
    struct pldm_msg* response =
        reinterpret_cast<struct pldm_msg*>(responseMsg.data());

    struct pldm_get_bios_attribute_current_value_by_handle_resp* resp =
        reinterpret_cast<
            struct pldm_get_bios_attribute_current_value_by_handle_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->next_transfer_handle = htole32(nextTransferHandle);
    resp->transfer_flag = transfer_flag;
    memcpy(resp->attribute_data, &attributeData, sizeof(attributeData));

    uint8_t retCompletionCode;
    uint32_t retNextTransferHandle;
    uint8_t retransfer_flag;
    struct variable_field retAttributeData;
    auto rc = decode_get_bios_attribute_current_value_by_handle_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retNextTransferHandle, &retransfer_flag, &retAttributeData);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, retCompletionCode);
    EXPECT_EQ(nextTransferHandle, retNextTransferHandle);
    EXPECT_EQ(transfer_flag, retransfer_flag);
    EXPECT_EQ(sizeof(attributeData), retAttributeData.length);
    EXPECT_EQ(
        0, memcmp(retAttributeData.ptr, &attributeData, sizeof(attributeData)));
}