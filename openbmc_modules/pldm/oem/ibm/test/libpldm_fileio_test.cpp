#include "libpldm/base.h"
#include "libpldm/file_io.h"

#include <string.h>

#include <array>

#include <gtest/gtest.h>

constexpr auto hdrSize = sizeof(pldm_msg_hdr);

TEST(ReadWriteFileMemory, testGoodDecodeRequest)
{
    std::array<uint8_t, PLDM_RW_FILE_MEM_REQ_BYTES + hdrSize> requestMsg{};

    // Random value for fileHandle, offset, length, address
    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x13245768;
    uint64_t address = 0x124356879ACBDE0F;
    uint32_t fileHandleLe = htole32(fileHandle);
    uint32_t offsetLe = htole32(offset);
    uint32_t lengthLe = htole32(length);
    uint64_t addressLe = htole64(address);

    memcpy(requestMsg.data() + hdrSize, &fileHandleLe, sizeof(fileHandleLe));
    memcpy(requestMsg.data() + sizeof(fileHandleLe) + hdrSize, &offsetLe,
           sizeof(offsetLe));
    memcpy(requestMsg.data() + sizeof(fileHandleLe) + sizeof(offsetLe) +
               hdrSize,
           &lengthLe, sizeof(lengthLe));
    memcpy(requestMsg.data() + sizeof(fileHandleLe) + sizeof(offsetLe) +
               sizeof(lengthLe) + hdrSize,
           &addressLe, sizeof(addressLe));

    uint32_t retFileHandle = 0;
    uint32_t retOffset = 0;
    uint32_t retLength = 0;
    uint64_t retAddress = 0;

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // Invoke decode the read file memory request
    auto rc = decode_rw_file_memory_req(request, requestMsg.size() - hdrSize,
                                        &retFileHandle, &retOffset, &retLength,
                                        &retAddress);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(fileHandle, retFileHandle);
    ASSERT_EQ(offset, retOffset);
    ASSERT_EQ(length, retLength);
    ASSERT_EQ(address, retAddress);
}

TEST(ReadWriteFileMemory, testBadDecodeRequest)
{
    uint32_t fileHandle = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
    uint64_t address = 0;

    // Request payload message is missing
    auto rc = decode_rw_file_memory_req(NULL, 0, &fileHandle, &offset, &length,
                                        &address);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_RW_FILE_MEM_REQ_BYTES> requestMsg{};

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // Address is NULL
    rc = decode_rw_file_memory_req(request, requestMsg.size() - hdrSize,
                                   &fileHandle, &offset, &length, NULL);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Payload length is invalid
    rc = decode_rw_file_memory_req(request, 0, &fileHandle, &offset, &length,
                                   &address);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadWriteFileMemory, testGoodEncodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_RW_FILE_MEM_RESP_BYTES>
        responseMsg{};
    uint32_t length = 0xFF00EE11;
    uint32_t lengthLe = htole32(length);
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // ReadFileIntoMemory
    auto rc = encode_rw_file_memory_resp(0, PLDM_READ_FILE_INTO_MEMORY,
                                         PLDM_SUCCESS, length, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_READ_FILE_INTO_MEMORY);
    ASSERT_EQ(response->payload[0], PLDM_SUCCESS);
    ASSERT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]),
                        &lengthLe, sizeof(lengthLe)));

    // WriteFileFromMemory
    rc = encode_rw_file_memory_resp(0, PLDM_WRITE_FILE_FROM_MEMORY,
                                    PLDM_SUCCESS, length, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_WRITE_FILE_FROM_MEMORY);
    ASSERT_EQ(response->payload[0], PLDM_SUCCESS);
    ASSERT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]),
                        &lengthLe, sizeof(lengthLe)));
}

TEST(ReadWriteFileMemory, testBadEncodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_RW_FILE_MEM_RESP_BYTES>
        responseMsg{};
    uint32_t length = 0;
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // ReadFileIntoMemory
    auto rc = encode_rw_file_memory_resp(0, PLDM_READ_FILE_INTO_MEMORY,
                                         PLDM_ERROR, length, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_READ_FILE_INTO_MEMORY);
    ASSERT_EQ(response->payload[0], PLDM_ERROR);

    // WriteFileFromMemory
    rc = encode_rw_file_memory_resp(0, PLDM_WRITE_FILE_FROM_MEMORY, PLDM_ERROR,
                                    length, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_WRITE_FILE_FROM_MEMORY);
    ASSERT_EQ(response->payload[0], PLDM_ERROR);
}

TEST(ReadWriteFileIntoMemory, testGoodDecodeResponse)
{
    std::array<uint8_t, PLDM_RW_FILE_MEM_RESP_BYTES + hdrSize> responseMsg{};
    // Random value for length
    uint32_t length = 0xFF00EE12;
    uint32_t lengthLe = htole32(length);
    uint8_t completionCode = 0;

    memcpy(responseMsg.data() + hdrSize, &completionCode,
           sizeof(completionCode));
    memcpy(responseMsg.data() + sizeof(completionCode) + hdrSize, &lengthLe,
           sizeof(lengthLe));

    uint8_t retCompletionCode = 0;
    uint32_t retLength = 0;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // Invoke decode the read file memory response
    auto rc = decode_rw_file_memory_resp(response, responseMsg.size() - hdrSize,
                                         &retCompletionCode, &retLength);
    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(completionCode, retCompletionCode);
    ASSERT_EQ(length, retLength);
}

TEST(ReadWriteFileIntoMemory, testBadDecodeResponse)
{
    uint32_t length = 0;
    uint8_t completionCode = 0;

    // Request payload message is missing
    auto rc = decode_rw_file_memory_resp(NULL, 0, &completionCode, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_RW_FILE_MEM_RESP_BYTES> responseMsg{};

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // Payload length is invalid
    rc = decode_rw_file_memory_resp(response, 0, &completionCode, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadWriteFileIntoMemory, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_RW_FILE_MEM_REQ_BYTES>
        requestMsg{};

    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x13245768;
    uint64_t address = 0x124356879ACBDE0F;
    uint32_t fileHandleLe = htole32(fileHandle);
    uint32_t offsetLe = htole32(offset);
    uint32_t lengthLe = htole32(length);
    uint64_t addressLe = htole64(address);

    pldm_msg* request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc =
        encode_rw_file_memory_req(0, PLDM_READ_FILE_INTO_MEMORY, fileHandle,
                                  offset, length, address, request);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(request->hdr.request, PLDM_REQUEST);
    ASSERT_EQ(request->hdr.instance_id, 0);
    ASSERT_EQ(request->hdr.type, PLDM_OEM);
    ASSERT_EQ(request->hdr.command, PLDM_READ_FILE_INTO_MEMORY);

    ASSERT_EQ(0, memcmp(request->payload, &fileHandleLe, sizeof(fileHandleLe)));

    ASSERT_EQ(0, memcmp(request->payload + sizeof(fileHandleLe), &offsetLe,
                        sizeof(offsetLe)));
    ASSERT_EQ(0,
              memcmp(request->payload + sizeof(fileHandleLe) + sizeof(offsetLe),
                     &lengthLe, sizeof(lengthLe)));
    ASSERT_EQ(0, memcmp(request->payload + sizeof(fileHandleLe) +
                            sizeof(offsetLe) + sizeof(lengthLe),
                        &addressLe, sizeof(addressLe)));
}

TEST(ReadWriteFileIntoMemory, testBadEncodeRequest)
{
    uint32_t fileHandle = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
    uint64_t address = 0;

    auto rc =
        encode_rw_file_memory_req(0, PLDM_READ_FILE_INTO_MEMORY, fileHandle,
                                  offset, length, address, NULL);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetFileTable, GoodDecodeRequest)
{
    std::array<uint8_t, PLDM_GET_FILE_TABLE_REQ_BYTES + hdrSize> requestMsg{};

    // Random value for DataTransferHandle, TransferOperationFlag, TableType
    uint32_t transferHandle = 0x12345678;
    uint32_t transferHandleLe = htole32(transferHandle);
    uint8_t transferOpFlag = 1;
    uint8_t tableType = 1;

    memcpy(requestMsg.data() + hdrSize, &transferHandleLe,
           sizeof(transferHandleLe));
    memcpy(requestMsg.data() + sizeof(transferHandle) + hdrSize,
           &transferOpFlag, sizeof(transferOpFlag));
    memcpy(requestMsg.data() + sizeof(transferHandle) + sizeof(transferOpFlag) +
               hdrSize,
           &tableType, sizeof(tableType));

    uint32_t retTransferHandle = 0;
    uint8_t retTransferOpFlag = 0;
    uint8_t retTableType = 0;

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // Invoke decode get file table request
    auto rc = decode_get_file_table_req(request, requestMsg.size() - hdrSize,
                                        &retTransferHandle, &retTransferOpFlag,
                                        &retTableType);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(transferHandle, retTransferHandle);
    ASSERT_EQ(transferOpFlag, retTransferOpFlag);
    ASSERT_EQ(tableType, retTableType);
}

TEST(GetFileTable, BadDecodeRequest)
{
    uint32_t transferHandle = 0;
    uint8_t transferOpFlag = 0;
    uint8_t tableType = 0;

    // Request payload message is missing
    auto rc = decode_get_file_table_req(nullptr, 0, &transferHandle,
                                        &transferOpFlag, &tableType);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_GET_FILE_TABLE_REQ_BYTES> requestMsg{};

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // TableType is NULL
    rc = decode_get_file_table_req(request, requestMsg.size() - hdrSize,
                                   &transferHandle, &transferOpFlag, nullptr);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Payload length is invalid
    rc = decode_get_file_table_req(request, 0, &transferHandle, &transferOpFlag,
                                   &tableType);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetFileTable, GoodEncodeResponse)
{
    // Random value for NextDataTransferHandle and TransferFlag
    uint8_t completionCode = 0;
    uint32_t nextTransferHandle = 0x87654321;
    uint32_t nextTransferHandleLe = htole32(nextTransferHandle);
    uint8_t transferFlag = 5;
    // Mock file table contents of size 5
    std::array<uint8_t, 5> fileTable = {1, 2, 3, 4, 5};
    constexpr size_t responseSize = sizeof(completionCode) +
                                    sizeof(nextTransferHandle) +
                                    sizeof(transferFlag) + fileTable.size();

    std::array<uint8_t, sizeof(pldm_msg_hdr) + responseSize> responseMsg{};
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // GetFileTable
    auto rc = encode_get_file_table_resp(0, PLDM_SUCCESS, nextTransferHandle,
                                         transferFlag, fileTable.data(),
                                         fileTable.size(), response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_GET_FILE_TABLE);
    ASSERT_EQ(response->payload[0], PLDM_SUCCESS);
    ASSERT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]),
                        &nextTransferHandleLe, sizeof(nextTransferHandle)));
    ASSERT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(nextTransferHandle),
                        &transferFlag, sizeof(transferFlag)));
    ASSERT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(nextTransferHandle),
                        &transferFlag, sizeof(transferFlag)));
    ASSERT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(nextTransferHandle) + sizeof(transferFlag),
                        fileTable.data(), fileTable.size()));
}

TEST(GetFileTable, BadEncodeResponse)
{
    uint8_t completionCode = 0;
    uint32_t nextTransferHandle = 0;
    uint8_t transferFlag = 0;
    constexpr size_t responseSize = sizeof(completionCode) +
                                    sizeof(nextTransferHandle) +
                                    sizeof(transferFlag);

    std::array<uint8_t, sizeof(pldm_msg_hdr) + responseSize> responseMsg{};
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // GetFileTable
    auto rc = encode_get_file_table_resp(0, PLDM_ERROR, nextTransferHandle,
                                         transferFlag, nullptr, 0, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_GET_FILE_TABLE);
    ASSERT_EQ(response->payload[0], PLDM_ERROR);
}

TEST(GetFileTable, GoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_FILE_TABLE_REQ_BYTES>
        requestMsg{};
    uint32_t transferHandle = 0x0;
    uint8_t transferOpFlag = 0x01;
    uint8_t tableType = PLDM_FILE_ATTRIBUTE_TABLE;

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = encode_get_file_table_req(0, transferHandle, transferOpFlag,
                                        tableType, request);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_get_file_table_req* req =
        reinterpret_cast<struct pldm_get_file_table_req*>(request->payload);
    EXPECT_EQ(transferHandle, le32toh(req->transfer_handle));
    EXPECT_EQ(transferOpFlag, req->operation_flag);
    EXPECT_EQ(tableType, req->table_type);
}

TEST(GetFileTable, BadEncodeRequest)
{
    uint32_t transferHandle = 0x0;
    uint8_t transferOpFlag = 0x01;
    uint8_t tableType = PLDM_FILE_ATTRIBUTE_TABLE;

    auto rc = encode_get_file_table_req(0, transferHandle, transferOpFlag,
                                        tableType, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetFileTable, GoodDecodeResponse)
{
    uint32_t nextTransferHandle = 32;
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag = PLDM_START_AND_END;
    std::vector<uint8_t> fileTableData = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    std::vector<uint8_t> responseMsg(
        hdrSize + PLDM_GET_FILE_TABLE_MIN_RESP_BYTES + fileTableData.size());

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());
    size_t payload_length = responseMsg.size() - hdrSize;

    auto resp = reinterpret_cast<struct pldm_get_file_table_resp*>(
        responsePtr->payload);

    resp->completion_code = completionCode;
    resp->next_transfer_handle = htole32(nextTransferHandle);
    resp->transfer_flag = transferFlag;
    memcpy(resp->table_data, fileTableData.data(), fileTableData.size());

    uint8_t retCompletionCode;
    uint32_t retNextTransferHandle;
    uint8_t retTransferFlag;
    std::vector<uint8_t> retFileTableData(9, 0);
    size_t retFileTableDataLength = 0;

    auto rc = decode_get_file_table_resp(
        responsePtr, payload_length, &retCompletionCode, &retNextTransferHandle,
        &retTransferFlag, retFileTableData.data(), &retFileTableDataLength);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(completionCode, retCompletionCode);
    ASSERT_EQ(nextTransferHandle, retNextTransferHandle);
    ASSERT_EQ(transferFlag, retTransferFlag);
    ASSERT_EQ(0, memcmp(fileTableData.data(), resp->table_data,
                        retFileTableDataLength));
    ASSERT_EQ(fileTableData.size(), retFileTableDataLength);
}

TEST(GetFileTable, BadDecodeResponse)
{
    uint32_t nextTransferHandle = 32;
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag = PLDM_START_AND_END;
    std::vector<uint8_t> fileTableData(9, 0);
    size_t file_table_data_length = 0;

    std::vector<uint8_t> responseMsg(
        hdrSize + PLDM_GET_FILE_TABLE_MIN_RESP_BYTES + fileTableData.size());

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_file_table_resp(
        nullptr, 0, &completionCode, &nextTransferHandle, &transferFlag,
        fileTableData.data(), &file_table_data_length);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_get_file_table_resp(
        responsePtr, 0, &completionCode, &nextTransferHandle, &transferFlag,
        fileTableData.data(), &file_table_data_length);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadFile, testGoodDecodeRequest)
{
    std::array<uint8_t, PLDM_READ_FILE_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};

    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());
    size_t payload_length = requestMsg.size() - sizeof(pldm_msg_hdr);
    auto request = reinterpret_cast<pldm_read_file_req*>(requestPtr->payload);

    // Random value for fileHandle, offset and length
    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x13245768;

    request->file_handle = htole32(fileHandle);
    request->offset = htole32(offset);
    request->length = htole32(length);

    uint32_t retFileHandle = 0;
    uint32_t retOffset = 0;
    uint32_t retLength = 0;

    // Invoke decode the read file request
    auto rc = decode_read_file_req(requestPtr, payload_length, &retFileHandle,
                                   &retOffset, &retLength);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(fileHandle, retFileHandle);
    ASSERT_EQ(offset, retOffset);
    ASSERT_EQ(length, retLength);
}

TEST(WriteFile, testGoodDecodeRequest)
{
    // Random value for fileHandle, offset, length and file data
    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x467;

    std::vector<uint8_t> requestMsg(PLDM_WRITE_FILE_REQ_BYTES +
                                    sizeof(pldm_msg_hdr) + length);
    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());
    size_t payload_length = requestMsg.size() - sizeof(pldm_msg_hdr);
    auto request = reinterpret_cast<pldm_write_file_req*>(requestPtr->payload);

    size_t fileDataOffset =
        sizeof(fileHandle) + sizeof(offset) + sizeof(length);

    request->file_handle = htole32(fileHandle);
    request->offset = htole32(offset);
    request->length = htole32(length);

    uint32_t retFileHandle = 0;
    uint32_t retOffset = 0;
    uint32_t retLength = 0;
    size_t retFileDataOffset = 0;

    // Invoke decode the write file request
    auto rc = decode_write_file_req(requestPtr, payload_length, &retFileHandle,
                                    &retOffset, &retLength, &retFileDataOffset);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(fileHandle, retFileHandle);
    ASSERT_EQ(offset, retOffset);
    ASSERT_EQ(length, retLength);
    ASSERT_EQ(fileDataOffset, retFileDataOffset);
}

TEST(ReadFile, testGoodDecodeResponse)
{
    // Random value for length
    uint32_t length = 0x10;
    uint8_t completionCode = PLDM_SUCCESS;

    std::vector<uint8_t> responseMsg(PLDM_READ_FILE_RESP_BYTES +
                                     sizeof(pldm_msg_hdr) + length);
    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());
    size_t payload_length = responseMsg.size() - sizeof(pldm_msg_hdr);
    auto response =
        reinterpret_cast<pldm_read_file_resp*>(responsePtr->payload);

    response->completion_code = completionCode;
    response->length = htole32(length);

    size_t fileDataOffset = sizeof(completionCode) + sizeof(length);

    uint32_t retLength = 0;
    uint8_t retCompletionCode = 0;
    size_t retFileDataOffset = 0;

    // Invoke decode the read file response
    auto rc =
        decode_read_file_resp(responsePtr, payload_length, &retCompletionCode,
                              &retLength, &retFileDataOffset);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(completionCode, retCompletionCode);
    ASSERT_EQ(length, retLength);
    ASSERT_EQ(fileDataOffset, retFileDataOffset);
}

TEST(WriteFile, testGoodDecodeResponse)
{
    std::array<uint8_t, PLDM_WRITE_FILE_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsg{};
    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());
    size_t payload_length = responseMsg.size() - sizeof(pldm_msg_hdr);
    auto response =
        reinterpret_cast<pldm_write_file_resp*>(responsePtr->payload);

    uint8_t completionCode = PLDM_SUCCESS;
    uint32_t length = 0x4678;

    response->completion_code = completionCode;
    response->length = htole32(length);

    uint32_t retLength = 0;
    uint8_t retCompletionCode = 0;

    // Invoke decode the write file response
    auto rc = decode_write_file_resp(responsePtr, payload_length,
                                     &retCompletionCode, &retLength);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(completionCode, retCompletionCode);
    ASSERT_EQ(length, retLength);
}

TEST(ReadWriteFile, testBadDecodeResponse)
{
    uint32_t length = 0;
    uint8_t completionCode = 0;
    size_t fileDataOffset = 0;

    // Bad decode response for read file
    std::vector<uint8_t> responseMsg(PLDM_READ_FILE_RESP_BYTES +
                                     sizeof(pldm_msg_hdr) + length);
    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // Request payload message is missing
    auto rc = decode_read_file_resp(NULL, 0, &completionCode, &length,
                                    &fileDataOffset);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Payload length is invalid
    rc = decode_read_file_resp(responsePtr, 0, &completionCode, &length,
                               &fileDataOffset);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    // Bad decode response for write file
    std::array<uint8_t, PLDM_WRITE_FILE_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsgWr{};
    auto responseWr = reinterpret_cast<pldm_msg*>(responseMsgWr.data());

    // Request payload message is missing
    rc = decode_write_file_resp(NULL, 0, &completionCode, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Payload length is invalid
    rc = decode_write_file_resp(responseWr, 0, &completionCode, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadWriteFile, testBadDecodeRequest)
{
    uint32_t fileHandle = 0;
    uint32_t offset = 0;
    uint32_t length = 0;

    // Bad decode request for read file
    std::array<uint8_t, PLDM_READ_FILE_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};
    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // Request payload message is missing
    auto rc = decode_read_file_req(NULL, 0, &fileHandle, &offset, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Payload length is invalid
    rc = decode_read_file_req(requestPtr, 0, &fileHandle, &offset, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    // Bad decode request for write file
    size_t fileDataOffset = 0;
    std::array<uint8_t, PLDM_WRITE_FILE_REQ_BYTES> requestMsgWr{};
    auto requestWr = reinterpret_cast<pldm_msg*>(requestMsgWr.data());

    // Request payload message is missing
    rc = decode_write_file_req(NULL, 0, &fileHandle, &offset, &length,
                               &fileDataOffset);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Payload length is invalid
    rc = decode_write_file_req(requestWr, 0, &fileHandle, &offset, &length,
                               &fileDataOffset);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadFile, testGoodEncodeResponse)
{
    // Good encode response for read file
    uint32_t length = 0x4;

    std::vector<uint8_t> responseMsg(PLDM_READ_FILE_RESP_BYTES +
                                     sizeof(pldm_msg_hdr) + length);
    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());
    auto response =
        reinterpret_cast<pldm_read_file_resp*>(responsePtr->payload);

    // ReadFile
    auto rc = encode_read_file_resp(0, PLDM_SUCCESS, length, responsePtr);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(responsePtr->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(responsePtr->hdr.instance_id, 0);
    ASSERT_EQ(responsePtr->hdr.type, PLDM_OEM);
    ASSERT_EQ(responsePtr->hdr.command, PLDM_READ_FILE);
    ASSERT_EQ(response->completion_code, PLDM_SUCCESS);
    ASSERT_EQ(le32toh(response->length), length);
}

TEST(WriteFile, testGoodEncodeResponse)
{
    uint32_t length = 0x467;

    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_WRITE_FILE_RESP_BYTES>
        responseMsg{};

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());
    auto response =
        reinterpret_cast<pldm_write_file_resp*>(responsePtr->payload);

    // WriteFile
    auto rc = encode_write_file_resp(0, PLDM_SUCCESS, length, responsePtr);
    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(responsePtr->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(responsePtr->hdr.instance_id, 0);
    ASSERT_EQ(responsePtr->hdr.type, PLDM_OEM);
    ASSERT_EQ(responsePtr->hdr.command, PLDM_WRITE_FILE);
    ASSERT_EQ(response->completion_code, PLDM_SUCCESS);
    ASSERT_EQ(le32toh(response->length), length);
}

TEST(ReadFile, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_READ_FILE_REQ_BYTES>
        requestMsg{};

    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x13245768;
    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto request = reinterpret_cast<pldm_read_file_req*>(requestPtr->payload);

    // ReadFile
    auto rc = encode_read_file_req(0, fileHandle, offset, length, requestPtr);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(requestPtr->hdr.request, PLDM_REQUEST);
    ASSERT_EQ(requestPtr->hdr.instance_id, 0);
    ASSERT_EQ(requestPtr->hdr.type, PLDM_OEM);
    ASSERT_EQ(requestPtr->hdr.command, PLDM_READ_FILE);
    ASSERT_EQ(le32toh(request->file_handle), fileHandle);
    ASSERT_EQ(le32toh(request->offset), offset);
    ASSERT_EQ(le32toh(request->length), length);
}

TEST(WriteFile, testGoodEncodeRequest)
{
    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x456;

    std::vector<uint8_t> requestMsg(PLDM_WRITE_FILE_REQ_BYTES +
                                    sizeof(pldm_msg_hdr) + length);
    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto request = reinterpret_cast<pldm_write_file_req*>(requestPtr->payload);

    // WriteFile
    auto rc = encode_write_file_req(0, fileHandle, offset, length, requestPtr);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(requestPtr->hdr.request, PLDM_REQUEST);
    ASSERT_EQ(requestPtr->hdr.instance_id, 0);
    ASSERT_EQ(requestPtr->hdr.type, PLDM_OEM);
    ASSERT_EQ(requestPtr->hdr.command, PLDM_WRITE_FILE);
    ASSERT_EQ(le32toh(request->file_handle), fileHandle);
    ASSERT_EQ(le32toh(request->offset), offset);
    ASSERT_EQ(le32toh(request->length), length);
}

TEST(ReadWriteFile, testBadEncodeRequest)
{
    // Bad encode request for read file
    uint32_t fileHandle = 0;
    uint32_t offset = 0;
    uint32_t length = 0;

    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_READ_FILE_REQ_BYTES>
        requestMsg{};
    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // ReadFile check invalid file length
    auto rc = encode_read_file_req(0, fileHandle, offset, length, requestPtr);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    // Bad encode request for write file
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_WRITE_FILE_REQ_BYTES>
        requestMsgWr{};
    auto requestWr = reinterpret_cast<pldm_msg*>(requestMsgWr.data());

    // WriteFile check for invalid file length
    rc = encode_write_file_req(0, fileHandle, offset, length, requestWr);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadWriteFile, testBadEncodeResponse)
{
    // Bad encode response for read file
    uint32_t length = 0;

    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_READ_FILE_RESP_BYTES>
        responseMsg{};
    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // ReadFile
    auto rc = encode_read_file_resp(0, PLDM_ERROR, length, responsePtr);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(responsePtr->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(responsePtr->hdr.instance_id, 0);
    ASSERT_EQ(responsePtr->hdr.type, PLDM_OEM);
    ASSERT_EQ(responsePtr->hdr.command, PLDM_READ_FILE);
    ASSERT_EQ(responsePtr->payload[0], PLDM_ERROR);

    // Bad encode response for write file
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_WRITE_FILE_RESP_BYTES>
        responseMsgWr{};
    auto responseWr = reinterpret_cast<pldm_msg*>(responseMsgWr.data());

    // WriteFile
    rc = encode_write_file_resp(0, PLDM_ERROR, length, responseWr);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(responseWr->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(responseWr->hdr.instance_id, 0);
    ASSERT_EQ(responseWr->hdr.type, PLDM_OEM);
    ASSERT_EQ(responseWr->hdr.command, PLDM_WRITE_FILE);
    ASSERT_EQ(responseWr->payload[0], PLDM_ERROR);
}

TEST(ReadWriteFileByTypeMemory, testGoodDecodeRequest)
{
    std::array<uint8_t,
               PLDM_RW_FILE_BY_TYPE_MEM_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};

    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());
    size_t payload_length = requestMsg.size() - sizeof(pldm_msg_hdr);
    auto request = reinterpret_cast<pldm_read_write_file_by_type_memory_req*>(
        requestPtr->payload);

    // Random value for fileHandle, offset and length
    uint16_t fileType = 0;
    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x13245768;
    uint64_t address = 0x124356879ACBD456;

    request->file_type = htole16(fileType);
    request->file_handle = htole32(fileHandle);
    request->offset = htole32(offset);
    request->length = htole32(length);
    request->address = htole64(address);

    uint16_t retFileType = 0x1;
    uint32_t retFileHandle = 0;
    uint32_t retOffset = 0;
    uint32_t retLength = 0;
    uint64_t retAddress = 0;

    // Invoke decode the read file request
    auto rc = decode_rw_file_by_type_memory_req(
        requestPtr, payload_length, &retFileType, &retFileHandle, &retOffset,
        &retLength, &retAddress);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(fileType, retFileType);
    ASSERT_EQ(fileHandle, retFileHandle);
    ASSERT_EQ(offset, retOffset);
    ASSERT_EQ(length, retLength);
    ASSERT_EQ(address, retAddress);
}

TEST(ReadWriteFileByTypeMemory, testGoodDecodeResponse)
{
    std::array<uint8_t,
               PLDM_RW_FILE_BY_TYPE_MEM_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsg{};

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());
    size_t payload_length = responseMsg.size() - sizeof(pldm_msg_hdr);
    auto response = reinterpret_cast<pldm_read_write_file_by_type_memory_resp*>(
        responsePtr->payload);

    // Random value for completion code and length
    uint8_t completionCode = 0x0;
    uint32_t length = 0x13245768;

    response->completion_code = completionCode;
    response->length = htole32(length);

    uint8_t retCompletionCode = 0x1;
    uint32_t retLength = 0;

    // Invoke decode the read/write file response
    auto rc = decode_rw_file_by_type_memory_resp(
        responsePtr, payload_length, &retCompletionCode, &retLength);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(completionCode, retCompletionCode);
    ASSERT_EQ(length, retLength);
}

TEST(ReadWriteFileByTypeMemory, testBadDecodeRequest)
{
    uint16_t fileType = 0;
    uint32_t fileHandle = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
    uint64_t address = 0;

    // Request payload message is missing
    auto rc = decode_rw_file_by_type_memory_req(NULL, 0, &fileType, &fileHandle,
                                                &offset, &length, &address);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t,
               PLDM_RW_FILE_BY_TYPE_MEM_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};

    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // Address is NULL
    rc = decode_rw_file_by_type_memory_req(
        requestPtr, requestMsg.size() - hdrSize, &fileType, &fileHandle,
        &offset, &length, NULL);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Payload length is invalid
    rc = decode_rw_file_by_type_memory_req(
        requestPtr, 0, &fileType, &fileHandle, &offset, &length, &address);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadWriteFileByTypeMemory, testBadDecodeResponse)
{
    uint32_t length = 0;
    uint8_t completionCode = 0;

    // Request payload message is missing
    auto rc =
        decode_rw_file_by_type_memory_resp(NULL, 0, &completionCode, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t,
               PLDM_RW_FILE_BY_TYPE_MEM_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsg{};

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // Length is NULL
    rc = decode_rw_file_by_type_memory_resp(
        responsePtr, responseMsg.size() - hdrSize, &completionCode, NULL);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Payload length is invalid
    rc = decode_rw_file_by_type_memory_resp(responsePtr, 0, &completionCode,
                                            &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadWriteFileByTypeMemory, testGoodEncodeRequest)
{
    std::array<uint8_t,
               sizeof(pldm_msg_hdr) + PLDM_RW_FILE_BY_TYPE_MEM_REQ_BYTES>
        requestMsg{};

    uint16_t fileType = 0;
    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x13245768;
    uint64_t address = 0x124356879ACBDE0F;
    uint16_t fileTypeLe = htole16(fileType);
    uint32_t fileHandleLe = htole32(fileHandle);
    uint32_t offsetLe = htole32(offset);
    uint32_t lengthLe = htole32(length);
    uint64_t addressLe = htole64(address);

    pldm_msg* request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_rw_file_by_type_memory_req(
        0, PLDM_READ_FILE_BY_TYPE_INTO_MEMORY, fileType, fileHandle, offset,
        length, address, request);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(request->hdr.request, PLDM_REQUEST);
    ASSERT_EQ(request->hdr.instance_id, 0);
    ASSERT_EQ(request->hdr.type, PLDM_OEM);
    ASSERT_EQ(request->hdr.command, PLDM_READ_FILE_BY_TYPE_INTO_MEMORY);

    ASSERT_EQ(0, memcmp(request->payload, &fileTypeLe, sizeof(fileTypeLe)));
    ASSERT_EQ(0, memcmp(request->payload + sizeof(fileTypeLe), &fileHandleLe,
                        sizeof(fileHandleLe)));

    ASSERT_EQ(
        0, memcmp(request->payload + sizeof(fileTypeLe) + sizeof(fileHandleLe),
                  &offsetLe, sizeof(offsetLe)));
    ASSERT_EQ(0, memcmp(request->payload + sizeof(fileTypeLe) +
                            sizeof(fileHandleLe) + sizeof(offsetLe),
                        &lengthLe, sizeof(lengthLe)));
    ASSERT_EQ(0, memcmp(request->payload + sizeof(fileTypeLe) +
                            sizeof(fileHandleLe) + sizeof(offsetLe) +
                            sizeof(lengthLe),
                        &addressLe, sizeof(addressLe)));
}

TEST(ReadWriteFileByTypeMemory, testGoodEncodeResponse)
{
    std::array<uint8_t,
               sizeof(pldm_msg_hdr) + PLDM_RW_FILE_BY_TYPE_MEM_RESP_BYTES>
        responseMsg{};

    uint32_t length = 0x13245768;
    uint32_t lengthLe = htole32(length);
    uint8_t completionCode = 0x0;

    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_rw_file_by_type_memory_resp(
        0, PLDM_READ_FILE_BY_TYPE_INTO_MEMORY, completionCode, length,
        response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_READ_FILE_BY_TYPE_INTO_MEMORY);

    ASSERT_EQ(
        0, memcmp(response->payload, &completionCode, sizeof(completionCode)));
    ASSERT_EQ(0, memcmp(response->payload + sizeof(completionCode), &lengthLe,
                        sizeof(lengthLe)));
}

TEST(ReadWriteFileByTypeMemory, testBadEncodeResponse)
{
    std::array<uint8_t,
               sizeof(pldm_msg_hdr) + PLDM_RW_FILE_BY_TYPE_MEM_RESP_BYTES>
        responseMsg{};
    uint32_t length = 0;
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // completion code is PLDM_ERROR
    auto rc = encode_rw_file_by_type_memory_resp(
        0, PLDM_READ_FILE_BY_TYPE_INTO_MEMORY, PLDM_ERROR, length, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_READ_FILE_BY_TYPE_INTO_MEMORY);
    ASSERT_EQ(response->payload[0], PLDM_ERROR);

    // response is NULL pointer
    rc = encode_rw_file_by_type_memory_resp(
        0, PLDM_READ_FILE_BY_TYPE_INTO_MEMORY, PLDM_SUCCESS, length, NULL);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(ReadWriteFileByTypeMemory, testBadEncodeRequest)
{
    uint8_t fileType = 0;
    uint32_t fileHandle = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
    uint64_t address = 0;

    // request is NULL pointer
    auto rc = encode_rw_file_by_type_memory_req(
        0, PLDM_READ_FILE_BY_TYPE_INTO_MEMORY, fileType, fileHandle, offset,
        length, address, NULL);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(NewFile, testGoodDecodeRequest)
{
    std::array<uint8_t, PLDM_NEW_FILE_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};

    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());
    size_t payload_length = requestMsg.size() - sizeof(pldm_msg_hdr);
    auto request = reinterpret_cast<pldm_new_file_req*>(requestPtr->payload);

    // Random value for fileHandle and length
    uint16_t fileType = 0xFF;
    uint32_t fileHandle = 0x12345678;
    uint64_t length = 0x13245768;

    request->file_type = htole16(fileType);
    request->file_handle = htole32(fileHandle);
    request->length = htole64(length);

    uint16_t retFileType = 0xFF;
    uint32_t retFileHandle = 0;
    uint64_t retLength = 0;

    // Invoke decode the read file request
    auto rc = decode_new_file_req(requestPtr, payload_length, &retFileType,
                                  &retFileHandle, &retLength);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(fileType, retFileType);
    ASSERT_EQ(fileHandle, retFileHandle);
    ASSERT_EQ(length, retLength);
}

TEST(NewFile, testGoodDecodeResponse)
{
    std::array<uint8_t, PLDM_NEW_FILE_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsg{};

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());
    size_t payload_length = responseMsg.size() - sizeof(pldm_msg_hdr);
    auto response = reinterpret_cast<pldm_new_file_resp*>(responsePtr->payload);

    // Random value for completion code
    uint8_t completionCode = 0x0;

    response->completion_code = completionCode;

    uint8_t retCompletionCode = PLDM_SUCCESS;

    // Invoke decode the read/write file response
    auto rc =
        decode_new_file_resp(responsePtr, payload_length, &retCompletionCode);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(completionCode, retCompletionCode);
}

TEST(NewFile, testBadDecodeRequest)
{
    uint16_t fileType = 0;
    uint32_t fileHandle = 0;
    uint64_t length = 0;

    // Request payload message is missing
    auto rc = decode_new_file_req(NULL, 0, &fileType, &fileHandle, &length);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_NEW_FILE_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};

    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // Payload length is invalid
    rc = decode_new_file_req(requestPtr, 0, &fileType, &fileHandle, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(NewFile, testBadDecodeResponse)
{
    uint8_t completionCode = 0;

    // Request payload message is missing
    auto rc = decode_new_file_resp(NULL, 0, &completionCode);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_NEW_FILE_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsg{};

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // Payload length is invalid
    rc = decode_new_file_resp(responsePtr, 0, &completionCode);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(NewFile, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_NEW_FILE_REQ_BYTES>
        requestMsg{};

    uint16_t fileType = 0xFF;
    uint32_t fileHandle = 0x12345678;
    uint32_t length = 0x13245768;
    uint16_t fileTypeLe = htole16(fileType);
    uint32_t fileHandleLe = htole32(fileHandle);
    uint32_t lengthLe = htole32(length);

    pldm_msg* request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_new_file_req(0, fileType, fileHandle, length, request);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(request->hdr.request, PLDM_REQUEST);
    ASSERT_EQ(request->hdr.instance_id, 0);
    ASSERT_EQ(request->hdr.type, PLDM_OEM);
    ASSERT_EQ(request->hdr.command, PLDM_NEW_FILE_AVAILABLE);
    ASSERT_EQ(0, memcmp(request->payload, &fileTypeLe, sizeof(fileTypeLe)));
    ASSERT_EQ(0, memcmp(request->payload + sizeof(fileTypeLe), &fileHandleLe,
                        sizeof(fileHandleLe)));
    ASSERT_EQ(
        0, memcmp(request->payload + sizeof(fileTypeLe) + sizeof(fileHandleLe),
                  &lengthLe, sizeof(lengthLe)));
}

TEST(NewFile, testGoodEncodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_NEW_FILE_RESP_BYTES>
        responseMsg{};

    uint8_t completionCode = 0x0;

    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_new_file_resp(0, completionCode, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_NEW_FILE_AVAILABLE);
    ASSERT_EQ(
        0, memcmp(response->payload, &completionCode, sizeof(completionCode)));
}

TEST(NewFile, testBadEncodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_NEW_FILE_RESP_BYTES>
        responseMsg{};
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // completion code is PLDM_ERROR
    auto rc = encode_new_file_resp(0, PLDM_ERROR, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_NEW_FILE_AVAILABLE);
    ASSERT_EQ(response->payload[0], PLDM_ERROR);

    // response is NULL pointer
    rc = encode_new_file_resp(0, PLDM_SUCCESS, NULL);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(NewFile, testBadEncodeRequest)
{
    uint8_t fileType = 0xFF;
    uint32_t fileHandle = 0;
    uint32_t length = 0;

    // request is NULL pointer
    auto rc = encode_new_file_req(0, fileType, fileHandle, length, NULL);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(ReadWriteFileByType, testGoodDecodeRequest)
{
    std::array<uint8_t, PLDM_RW_FILE_BY_TYPE_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};

    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());
    size_t payload_length = requestMsg.size() - sizeof(pldm_msg_hdr);
    auto request = reinterpret_cast<pldm_read_write_file_by_type_req*>(
        requestPtr->payload);

    // Random value for fileHandle, offset and length
    uint16_t fileType = 0;
    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x13245768;

    request->file_handle = htole32(fileHandle);
    request->offset = htole32(offset);
    request->length = htole32(length);

    uint16_t retFileType = 0x1;
    uint32_t retFileHandle = 0;
    uint32_t retOffset = 0;
    uint32_t retLength = 0;

    // Invoke decode the read file request
    auto rc =
        decode_rw_file_by_type_req(requestPtr, payload_length, &retFileType,
                                   &retFileHandle, &retOffset, &retLength);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(fileType, retFileType);
    ASSERT_EQ(fileHandle, retFileHandle);
    ASSERT_EQ(offset, retOffset);
    ASSERT_EQ(length, retLength);
}

TEST(ReadWriteFileByType, testGoodDecodeResponse)
{
    std::array<uint8_t, PLDM_RW_FILE_BY_TYPE_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsg{};

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());
    size_t payload_length = responseMsg.size() - sizeof(pldm_msg_hdr);
    auto response = reinterpret_cast<pldm_read_write_file_by_type_resp*>(
        responsePtr->payload);

    // Random value for completion code and length
    uint8_t completionCode = 0x0;
    uint32_t length = 0x13245768;

    response->completion_code = completionCode;
    response->length = htole32(length);

    uint8_t retCompletionCode = 0x1;
    uint32_t retLength = 0;

    // Invoke decode the read/write file response
    auto rc = decode_rw_file_by_type_resp(responsePtr, payload_length,
                                          &retCompletionCode, &retLength);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(completionCode, retCompletionCode);
    ASSERT_EQ(length, retLength);
}

TEST(ReadWriteFileByType, testBadDecodeRequest)
{
    uint16_t fileType = 0;
    uint32_t fileHandle = 0;
    uint32_t offset = 0;
    uint32_t length = 0;

    // Request payload message is missing
    auto rc = decode_rw_file_by_type_req(NULL, 0, &fileType, &fileHandle,
                                         &offset, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_RW_FILE_BY_TYPE_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};

    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // Payload length is invalid
    rc = decode_rw_file_by_type_req(requestPtr, 0, &fileType, &fileHandle,
                                    &offset, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadWriteFileByType, testBadDecodeResponse)
{
    uint32_t length = 0;
    uint8_t completionCode = 0;

    // Request payload message is missing
    auto rc = decode_rw_file_by_type_resp(NULL, 0, &completionCode, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_RW_FILE_BY_TYPE_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsg{};

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // Length is NULL
    rc = decode_rw_file_by_type_resp(responsePtr, responseMsg.size() - hdrSize,
                                     &completionCode, NULL);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Payload length is invalid
    rc = decode_rw_file_by_type_resp(responsePtr, 0, &completionCode, &length);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(ReadWriteFileByType, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_RW_FILE_BY_TYPE_REQ_BYTES>
        requestMsg{};

    uint16_t fileType = 0;
    uint32_t fileHandle = 0x12345678;
    uint32_t offset = 0x87654321;
    uint32_t length = 0x13245768;
    uint16_t fileTypeLe = htole16(fileType);
    uint32_t fileHandleLe = htole32(fileHandle);
    uint32_t offsetLe = htole32(offset);
    uint32_t lengthLe = htole32(length);

    pldm_msg* request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_rw_file_by_type_req(0, PLDM_READ_FILE_BY_TYPE, fileType,
                                         fileHandle, offset, length, request);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(request->hdr.request, PLDM_REQUEST);
    ASSERT_EQ(request->hdr.instance_id, 0);
    ASSERT_EQ(request->hdr.type, PLDM_OEM);
    ASSERT_EQ(request->hdr.command, PLDM_READ_FILE_BY_TYPE);

    ASSERT_EQ(0, memcmp(request->payload, &fileTypeLe, sizeof(fileTypeLe)));
    ASSERT_EQ(0, memcmp(request->payload + sizeof(fileTypeLe), &fileHandleLe,
                        sizeof(fileHandleLe)));

    ASSERT_EQ(
        0, memcmp(request->payload + sizeof(fileTypeLe) + sizeof(fileHandleLe),
                  &offsetLe, sizeof(offsetLe)));
    ASSERT_EQ(0, memcmp(request->payload + sizeof(fileTypeLe) +
                            sizeof(fileHandleLe) + sizeof(offsetLe),
                        &lengthLe, sizeof(lengthLe)));
}

TEST(ReadWriteFileByType, testGoodEncodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_RW_FILE_BY_TYPE_RESP_BYTES>
        responseMsg{};

    uint32_t length = 0x13245768;
    uint32_t lengthLe = htole32(length);
    uint8_t completionCode = 0x0;
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_rw_file_by_type_resp(0, PLDM_READ_FILE_BY_TYPE,
                                          completionCode, length, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_READ_FILE_BY_TYPE);

    ASSERT_EQ(
        0, memcmp(response->payload, &completionCode, sizeof(completionCode)));
    ASSERT_EQ(0, memcmp(response->payload + sizeof(completionCode), &lengthLe,
                        sizeof(lengthLe)));
}

TEST(ReadWriteFileByType, testBadEncodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_RW_FILE_BY_TYPE_RESP_BYTES>
        responseMsg{};
    uint32_t length = 0;
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // completion code is PLDM_ERROR
    auto rc = encode_rw_file_by_type_resp(0, PLDM_READ_FILE_BY_TYPE, PLDM_ERROR,
                                          length, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_READ_FILE_BY_TYPE);
    ASSERT_EQ(response->payload[0], PLDM_ERROR);

    // response is NULL pointer
    rc = encode_rw_file_by_type_resp(0, PLDM_READ_FILE_BY_TYPE, PLDM_SUCCESS,
                                     length, NULL);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(ReadWriteFileByType, testBadEncodeRequest)
{
    uint8_t fileType = 0;
    uint32_t fileHandle = 0;
    uint32_t offset = 0;
    uint32_t length = 0;

    // request is NULL pointer
    auto rc = encode_rw_file_by_type_req(0, PLDM_READ_FILE_BY_TYPE, fileType,
                                         fileHandle, offset, length, NULL);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(FileAck, testGoodDecodeRequest)
{
    std::array<uint8_t, PLDM_FILE_ACK_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};

    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());
    size_t payload_length = requestMsg.size() - sizeof(pldm_msg_hdr);
    auto request = reinterpret_cast<pldm_file_ack_req*>(requestPtr->payload);

    // Random value for fileHandle
    uint16_t fileType = 0xFFFF;
    uint32_t fileHandle = 0x12345678;
    uint8_t fileStatus = 0xFF;

    request->file_type = htole16(fileType);
    request->file_handle = htole32(fileHandle);
    request->file_status = fileStatus;

    uint16_t retFileType = 0xFF;
    uint32_t retFileHandle = 0;
    uint8_t retFileStatus = 0;

    // Invoke decode the read file request
    auto rc = decode_file_ack_req(requestPtr, payload_length, &retFileType,
                                  &retFileHandle, &retFileStatus);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(fileType, retFileType);
    ASSERT_EQ(fileHandle, retFileHandle);
    ASSERT_EQ(fileStatus, retFileStatus);
}

TEST(FileAck, testGoodDecodeResponse)
{
    std::array<uint8_t, PLDM_FILE_ACK_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsg{};

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());
    size_t payload_length = responseMsg.size() - sizeof(pldm_msg_hdr);
    auto response = reinterpret_cast<pldm_file_ack_resp*>(responsePtr->payload);

    // Random value for completion code
    uint8_t completionCode = 0x0;

    response->completion_code = completionCode;

    uint8_t retCompletionCode = PLDM_SUCCESS;

    // Invoke decode the read/write file response
    auto rc =
        decode_file_ack_resp(responsePtr, payload_length, &retCompletionCode);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(completionCode, retCompletionCode);
}

TEST(FileAck, testBadDecodeRequest)
{
    uint16_t fileType = 0;
    uint32_t fileHandle = 0;
    uint8_t fileStatus = 0;

    // Request payload message is missing
    auto rc = decode_file_ack_req(NULL, 0, &fileType, &fileHandle, &fileStatus);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_FILE_ACK_REQ_BYTES + sizeof(pldm_msg_hdr)>
        requestMsg{};

    auto requestPtr = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // Payload length is invalid
    rc =
        decode_file_ack_req(requestPtr, 0, &fileType, &fileHandle, &fileStatus);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(FileAck, testBadDecodeResponse)
{
    uint8_t completionCode = 0;

    // Request payload message is missing
    auto rc = decode_file_ack_resp(NULL, 0, &completionCode);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_FILE_ACK_RESP_BYTES + sizeof(pldm_msg_hdr)>
        responseMsg{};

    auto responsePtr = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // Payload length is invalid
    rc = decode_file_ack_resp(responsePtr, 0, &completionCode);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(FileAck, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_FILE_ACK_REQ_BYTES>
        requestMsg{};

    uint16_t fileType = 0xFFFF;
    uint32_t fileHandle = 0x12345678;
    uint8_t fileStatus = 0xFF;
    uint16_t fileTypeLe = htole16(fileType);
    uint32_t fileHandleLe = htole32(fileHandle);

    pldm_msg* request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_file_ack_req(0, fileType, fileHandle, fileStatus, request);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(request->hdr.request, PLDM_REQUEST);
    ASSERT_EQ(request->hdr.instance_id, 0);
    ASSERT_EQ(request->hdr.type, PLDM_OEM);
    ASSERT_EQ(request->hdr.command, PLDM_FILE_ACK);
    ASSERT_EQ(0, memcmp(request->payload, &fileTypeLe, sizeof(fileTypeLe)));
    ASSERT_EQ(0, memcmp(request->payload + sizeof(fileTypeLe), &fileHandleLe,
                        sizeof(fileHandleLe)));
}

TEST(FileAck, testGoodEncodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_FILE_ACK_RESP_BYTES>
        responseMsg{};

    uint8_t completionCode = 0x0;

    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_file_ack_resp(0, completionCode, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_FILE_ACK);
    ASSERT_EQ(
        0, memcmp(response->payload, &completionCode, sizeof(completionCode)));
}

TEST(FileAck, testBadEncodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_FILE_ACK_RESP_BYTES>
        responseMsg{};
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // completion code is PLDM_ERROR
    auto rc = encode_file_ack_resp(0, PLDM_ERROR, response);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    ASSERT_EQ(response->hdr.request, PLDM_RESPONSE);
    ASSERT_EQ(response->hdr.instance_id, 0);
    ASSERT_EQ(response->hdr.type, PLDM_OEM);
    ASSERT_EQ(response->hdr.command, PLDM_FILE_ACK);
    ASSERT_EQ(response->payload[0], PLDM_ERROR);

    // response is NULL pointer
    rc = encode_file_ack_resp(0, PLDM_SUCCESS, NULL);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(FileAck, testBadEncodeRequest)
{
    uint8_t fileType = 0xFF;
    uint32_t fileHandle = 0;
    uint8_t fileStatus = 0;

    // request is NULL pointer
    auto rc = encode_file_ack_req(0, fileType, fileHandle, fileStatus, nullptr);

    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}
