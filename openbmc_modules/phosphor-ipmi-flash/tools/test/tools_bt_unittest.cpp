#include "bt.hpp"
#include "internal_sys_mock.hpp"
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
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Return;

TEST(BtHandlerTest, verifySendsFileContents)
{
    /* In this very basic test, we'll feed the bt handler data from the internal
     * syscall mock and catch the writes via the blob mock.
     */
    internal::InternalSysMock sysMock;
    ipmiblob::BlobInterfaceMock blobMock;
    ProgressMock progMock;

    BtDataHandler handler(&blobMock, &progMock, &sysMock);
    std::string filePath = "/asdf";
    int fd = 1;
    std::uint16_t session = 0xbeef;
    std::vector<std::uint8_t> bytes = {'1', '2', '3', '4'};
    const int fakeFileSize = 100;

    EXPECT_CALL(sysMock, open(Eq(filePath), _)).WillOnce(Return(fd));
    EXPECT_CALL(sysMock, getSize(Eq(filePath))).WillOnce(Return(fakeFileSize));
    EXPECT_CALL(sysMock, read(fd, NotNull(), _))
        .WillOnce(Invoke([&](int fd, void* buf, std::size_t count) {
            EXPECT_TRUE(count > bytes.size());
            std::memcpy(buf, bytes.data(), bytes.size());
            return bytes.size();
        }))
        .WillOnce(Return(0));
    EXPECT_CALL(sysMock, close(fd)).WillOnce(Return(0));

    EXPECT_CALL(blobMock, writeBytes(session, 0, ContainerEq(bytes)));

    EXPECT_TRUE(handler.sendContents(filePath, session));
}

} // namespace
} // namespace host_tool
