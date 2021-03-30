#include <ipmiblob/blob_errors.hpp>
#include <ipmiblob/blob_handler.hpp>
#include <ipmiblob/test/crc_mock.hpp>
#include <ipmiblob/test/ipmi_interface_mock.hpp>

#include <cstdint>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

namespace ipmiblob
{
CrcInterface* crcIntf = nullptr;

std::uint16_t generateCrc(const std::vector<std::uint8_t>& data)
{
    return (crcIntf) ? crcIntf->generateCrc(data) : 0x00;
}

using ::testing::ContainerEq;
using ::testing::Eq;
using ::testing::Return;

class BlobHandlerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        crcIntf = &crcMock;
    }

    CrcMock crcMock;

    std::unique_ptr<IpmiInterface> CreateIpmiMock()
    {
        return std::make_unique<IpmiInterfaceMock>();
    }
};

TEST_F(BlobHandlerTest, getCountIpmiHappy)
{
    /* Verify returns the value specified by the IPMI response. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2, 0x00,
        static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobGetCount)};

    /* return 1 blob count. */
    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00, 0x00, 0x00,
                                      0x01, 0x00, 0x00, 0x00};

    std::vector<std::uint8_t> bytes = {0x01, 0x00, 0x00, 0x00};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(bytes)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));
    EXPECT_EQ(1, blob.getBlobCount());
}

TEST_F(BlobHandlerTest, enumerateBlobIpmiHappy)
{
    /* Verify returns the name specified by the IPMI response. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobEnumerate),
        0x00, 0x00,
        0x01, 0x00,
        0x00, 0x00};

    /* return value. */
    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00, 0x00, 0x00,
                                      'a',  'b',  'c',  'd',  0x00};

    std::vector<std::uint8_t> bytes = {'a', 'b', 'c', 'd', 0x00};
    std::vector<std::uint8_t> reqCrc = {0x01, 0x00, 0x00, 0x00};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(bytes)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));
    EXPECT_STREQ("abcd", blob.enumerateBlob(1).c_str());
}

TEST_F(BlobHandlerTest, enumerateBlobIpmiNoBytes)
{
    /* Simulate a case where the IPMI command returns no data. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobEnumerate),
        0x00, 0x00,
        0x01, 0x00,
        0x00, 0x00};

    /* return value. */
    std::vector<std::uint8_t> resp = {};

    std::vector<std::uint8_t> reqCrc = {0x01, 0x00, 0x00, 0x00};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));
    EXPECT_STREQ("", blob.enumerateBlob(1).c_str());
}

TEST_F(BlobHandlerTest, getBlobListIpmiHappy)
{
    /* Verify returns the list built via the above two commands. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request1 = {
        0xcf, 0xc2, 0x00,
        static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobGetCount)};

    /* return 1 blob count. */
    std::vector<std::uint8_t> resp1 = {0xcf, 0xc2, 0x00, 0x00, 0x00,
                                       0x01, 0x00, 0x00, 0x00};

    std::vector<std::uint8_t> bytes1 = {0x01, 0x00, 0x00, 0x00};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(bytes1)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request1)))
        .WillOnce(Return(resp1));

    std::vector<std::uint8_t> request2 = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobEnumerate),
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00};

    /* return value. */
    std::vector<std::uint8_t> resp2 = {0xcf, 0xc2, 0x00, 0x00, 0x00,
                                       'a',  'b',  'c',  'd',  0x00};

    std::vector<std::uint8_t> reqCrc = {0x00, 0x00, 0x00, 0x00};
    std::vector<std::uint8_t> bytes2 = {'a', 'b', 'c', 'd', 0x00};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(bytes2)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request2)))
        .WillOnce(Return(resp2));

    /* A std::string is not nul-terminated by default. */
    std::vector<std::string> expectedList = {std::string{"abcd"}};

    EXPECT_EQ(expectedList, blob.getBlobList());
}

TEST_F(BlobHandlerTest, getStatWithMetadata)
{
    /* Stat received metadata. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobStat),
        0x00, 0x00,
        'a',  'b',
        'c',  'd',
        0x00};

    /* return blob_state: 0xffff, size: 0x00, metadata 0x3445 */
    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00, 0x00, 0x00, 0xff, 0xff,
                                      0x00, 0x00, 0x00, 0x00, 0x02, 0x34, 0x45};

    std::vector<std::uint8_t> reqCrc = {'a', 'b', 'c', 'd', 0x00};
    std::vector<std::uint8_t> respCrc = {0xff, 0xff, 0x00, 0x00, 0x00,
                                         0x00, 0x02, 0x34, 0x45};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(respCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    auto meta = blob.getStat("abcd");
    EXPECT_EQ(meta.blob_state, 0xffff);
    EXPECT_EQ(meta.size, 0x00);
    std::vector<std::uint8_t> metadata = {0x34, 0x45};
    EXPECT_EQ(metadata, meta.metadata);
}

TEST_F(BlobHandlerTest, getStatWithWrongMetadataLength)
{
    /* Stat fails when wrong metadata length is received */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobStat),
        0x00, 0x00,
        'a',  'b',
        'c',  'd',
        0x00};

    /* return blob_state: 0xffff, size: 0x00, len: 1, metadata 0x3445 */
    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00, 0x00, 0x00, 0xff, 0xff,
                                      0x00, 0x00, 0x00, 0x00, 0x01, 0x34, 0x45};

    std::vector<std::uint8_t> reqCrc = {'a', 'b', 'c', 'd', 0x00};
    std::vector<std::uint8_t> respCrc = {0xff, 0xff, 0x00, 0x00, 0x00,
                                         0x00, 0x01, 0x34, 0x45};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(respCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    EXPECT_THROW(blob.getStat("abcd"), BlobException);
}

TEST_F(BlobHandlerTest, getStatNoMetadata)
{
    /* Stat received no metadata. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobStat),
        0x00, 0x00,
        'a',  'b',
        'c',  'd',
        0x00};

    /* return blob_state: 0xffff, size: 0x00, metadata 0x0000 */
    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00, 0x00, 0x00, 0xff,
                                      0xff, 0x00, 0x00, 0x00, 0x00, 0x00};

    std::vector<std::uint8_t> reqCrc = {'a', 'b', 'c', 'd', 0x00};
    std::vector<std::uint8_t> respCrc = {0xff, 0xff, 0x00, 0x00,
                                         0x00, 0x00, 0x00};

    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(respCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    auto meta = blob.getStat("abcd");
    EXPECT_EQ(meta.blob_state, 0xffff);
    EXPECT_EQ(meta.size, 0x00);
    std::vector<std::uint8_t> metadata = {};
    EXPECT_EQ(metadata, meta.metadata);
}

TEST_F(BlobHandlerTest, getSessionStatNoMetadata)
{
    /* The get session stat succeeds. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobSessionStat),
        0x00, 0x00,
        0x01, 0x00};

    /* return blob_state: 0xffff, size: 0x00, metadata 0x0000 */
    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00, 0x00, 0x00, 0xff,
                                      0xff, 0x00, 0x00, 0x00, 0x00, 0x00};

    std::vector<std::uint8_t> reqCrc = {0x01, 0x00};
    std::vector<std::uint8_t> respCrc = {0xff, 0xff, 0x00, 0x00,
                                         0x00, 0x00, 0x00};

    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(respCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    auto meta = blob.getStat(0x0001);
    EXPECT_EQ(meta.blob_state, 0xffff);
    EXPECT_EQ(meta.size, 0x00);
    std::vector<std::uint8_t> metadata = {};
    EXPECT_EQ(metadata, meta.metadata);
}

TEST_F(BlobHandlerTest, getSessionStatEmptyResp)
{
    /* The get session stat fails after getting empty response */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobSessionStat),
        0x00, 0x00,
        0x01, 0x00};

    std::vector<std::uint8_t> resp;
    std::vector<std::uint8_t> reqCrc = {0x01, 0x00};

    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    EXPECT_THROW(blob.getStat(0x0001), BlobException);
}

TEST_F(BlobHandlerTest, getSessionStatInvalidHeader)
{
    /* The get session stat fails after getting a response shorter than the
     * expected headersize but with the expected OEN
     */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobSessionStat),
        0x00, 0x00,
        0x01, 0x00};

    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00, 0x00};
    std::vector<std::uint8_t> reqCrc = {0x01, 0x00};

    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    EXPECT_THROW(blob.getStat(0x0001), BlobException);
}

TEST_F(BlobHandlerTest, openBlobSucceeds)
{
    /* The open blob succeeds. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobOpen),
        0x00, 0x00,
        0x02, 0x04,
        'a',  'b',
        'c',  'd',
        0x00};

    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00, 0x00, 0x00, 0xfe, 0xed};

    std::vector<std::uint8_t> reqCrc = {0x02, 0x04, 'a', 'b', 'c', 'd', 0x00};
    std::vector<std::uint8_t> respCrc = {0xfe, 0xed};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(respCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    const int writeBit = (1 << 1);
    const int lpcBit = (1 << 10);

    auto session = blob.openBlob("abcd", writeBit | lpcBit);
    EXPECT_EQ(0xedfe, session);
}

TEST_F(BlobHandlerTest, closeBlobSucceeds)
{
    /* The close succeeds. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobClose),
        0x00, 0x00,
        0x01, 0x00};
    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00};
    std::vector<std::uint8_t> reqCrc = {0x01, 0x00};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    blob.closeBlob(0x0001);
}

TEST_F(BlobHandlerTest, commitSucceedsNoData)
{
    /* The commit succeeds. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobCommit),
        0x00, 0x00,
        0x01, 0x00,
        0x00};

    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00};
    std::vector<std::uint8_t> reqCrc = {0x01, 0x00, 0x00};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    blob.commit(0x0001, {});
}

TEST_F(BlobHandlerTest, writeBytesSucceeds)
{
    /* The write bytes succeeds. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobWrite),
        0x00, 0x00,
        0x01, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        'a',  'b',
        'c',  'd'};

    std::vector<std::uint8_t> bytes = {'a', 'b', 'c', 'd'};
    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00};
    std::vector<std::uint8_t> reqCrc = {0x01, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 'a',  'b',  'c',  'd'};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    blob.writeBytes(0x0001, 0, bytes);
}

TEST_F(BlobHandlerTest, readBytesSucceeds)
{
    /* The reading of bytes succeeds. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobRead),
        0x00, 0x00,
        0x01, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x04, 0x00,
        0x00, 0x00};

    std::vector<std::uint8_t> expectedBytes = {'a', 'b', 'c', 'd'};
    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00, 0x00, 0x00,
                                      'a',  'b',  'c',  'd'};
    std::vector<std::uint8_t> reqCrc = {0x01, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x04, 0x00, 0x00, 0x00};
    std::vector<std::uint8_t> respCrc = {'a', 'b', 'c', 'd'};

    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(respCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    EXPECT_EQ(blob.readBytes(0x0001, 0, 4), expectedBytes);
}

TEST_F(BlobHandlerTest, deleteBlobSucceeds)
{
    /* The delete succeeds. */
    auto ipmi = CreateIpmiMock();
    IpmiInterfaceMock* ipmiMock =
        reinterpret_cast<IpmiInterfaceMock*>(ipmi.get());
    BlobHandler blob(std::move(ipmi));

    std::vector<std::uint8_t> request = {
        0xcf, 0xc2,
        0x00, static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobDelete),
        0x00, 0x00,
        'a',  'b',
        'c',  'd',
        0x00};

    std::vector<std::uint8_t> resp = {0xcf, 0xc2, 0x00};
    std::vector<std::uint8_t> reqCrc = {'a', 'b', 'c', 'd', 0x00};
    EXPECT_CALL(crcMock, generateCrc(ContainerEq(reqCrc)))
        .WillOnce(Return(0x00));

    EXPECT_CALL(*ipmiMock,
                sendPacket(ipmiOEMNetFn, ipmiOEMBlobCmd, ContainerEq(request)))
        .WillOnce(Return(resp));

    blob.deleteBlob("abcd");
}

} // namespace ipmiblob
