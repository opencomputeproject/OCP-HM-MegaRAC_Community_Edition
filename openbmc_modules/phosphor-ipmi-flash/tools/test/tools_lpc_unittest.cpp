#include "internal_sys_mock.hpp"
#include "io_mock.hpp"
#include "lpc.hpp"
#include "progress_mock.hpp"

#include <ipmiblob/test/blob_interface_mock.hpp>

#include <cstring>

#include <gtest/gtest.h>

namespace host_tool
{
namespace
{

using ::testing::_;
using ::testing::ContainerEq;
using ::testing::Gt;
using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;

TEST(LpcHandleTest, verifySendsFileContents)
{
    internal::InternalSysMock sysMock;
    ipmiblob::BlobInterfaceMock blobMock;
    HostIoInterfaceMock ioMock;
    ProgressMock progMock;

    const std::uint32_t address = 0xfedc1000;
    const std::uint32_t length = 0x1000;

    LpcDataHandler handler(&blobMock, &ioMock, address, length, &progMock,
                           &sysMock);
    std::uint16_t session = 0xbeef;
    std::string filePath = "/asdf";
    int fileDescriptor = 5;
    const int fakeFileSize = 100;

    LpcRegion host_lpc_buf;
    host_lpc_buf.address = address;
    host_lpc_buf.length = length;

    std::vector<std::uint8_t> bytes(sizeof(host_lpc_buf));
    std::memcpy(bytes.data(), &host_lpc_buf, sizeof(host_lpc_buf));

    EXPECT_CALL(blobMock, writeMeta(session, 0, ContainerEq(bytes)));

    std::vector<std::uint8_t> data = {0x01, 0x02, 0x03};

    EXPECT_CALL(sysMock, open(StrEq(filePath.c_str()), _))
        .WillOnce(Return(fileDescriptor));
    EXPECT_CALL(sysMock, getSize(StrEq(filePath.c_str())))
        .WillOnce(Return(fakeFileSize));
    EXPECT_CALL(sysMock, read(_, NotNull(), Gt(data.size())))
        .WillOnce(Invoke([&data](int, void* buf, std::size_t) {
            std::memcpy(buf, data.data(), data.size());
            return data.size();
        }))
        .WillOnce(Return(0));

    EXPECT_CALL(ioMock, write(_, data.size(), _))
        .WillOnce(Invoke([&data](const std::size_t, const std::size_t length,
                                 const void* const source) {
            EXPECT_THAT(std::memcmp(source, data.data(), data.size()), 0);
            return true;
        }));

    EXPECT_CALL(blobMock, writeBytes(session, 0, _));

    EXPECT_CALL(sysMock, close(fileDescriptor)).WillOnce(Return(0));

    EXPECT_TRUE(handler.sendContents(filePath, session));
}

} // namespace
} // namespace host_tool
