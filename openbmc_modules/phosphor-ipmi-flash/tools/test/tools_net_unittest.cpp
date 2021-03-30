#include "data.hpp"
#include "internal_sys_mock.hpp"
#include "net.hpp"
#include "progress_mock.hpp"

#include <ipmiblob/test/blob_interface_mock.hpp>

#include <cstring>

#include <gtest/gtest.h>

namespace host_tool
{
namespace
{

using namespace std::literals;

using ::testing::_;
using ::testing::AllOf;
using ::testing::ContainerEq;
using ::testing::DoAll;
using ::testing::Field;
using ::testing::Gt;
using ::testing::InSequence;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetErrnoAndReturn;
using ::testing::StrEq;

class NetHandleTest : public ::testing::Test
{
  protected:
    NetHandleTest() : handler(&blobMock, &progMock, host, port, &sysMock)
    {
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(622);
        sa.sin6_flowinfo = 0;
        sa.sin6_addr = in6addr_loopback; // ::1
        sa.sin6_scope_id = 0;

        addr.ai_family = AF_INET6;
        addr.ai_socktype = SOCK_STREAM;
        addr.ai_addr = reinterpret_cast<struct sockaddr*>(&sa);
        addr.ai_addrlen = sizeof(sa);
        addr.ai_protocol = 0;
        addr.ai_next = nullptr;
    }

    void expectOpenFile()
    {
        EXPECT_CALL(sysMock, open(StrEq(filePath.c_str()), _))
            .WillOnce(Return(inFd));
        EXPECT_CALL(sysMock, close(inFd)).WillOnce(Return(0));
        EXPECT_CALL(sysMock, getSize(StrEq(filePath.c_str())))
            .WillOnce(Return(fakeFileSize));

        EXPECT_CALL(progMock, start(fakeFileSize));
    }

    void expectAddrInfo()
    {
        EXPECT_CALL(
            sysMock,
            getaddrinfo(StrEq(host), StrEq(port),
                        AllOf(Field(&addrinfo::ai_flags, AI_NUMERICHOST),
                              Field(&addrinfo::ai_family, AF_UNSPEC),
                              Field(&addrinfo::ai_socktype, SOCK_STREAM)),
                        NotNull()))
            .WillOnce(DoAll(SetArgPointee<3>(&addr), Return(0)));
        EXPECT_CALL(sysMock, freeaddrinfo(&addr));
    }

    void expectConnection()
    {
        EXPECT_CALL(sysMock, socket(AF_INET6, SOCK_STREAM, 0))
            .WillOnce(Return(connFd));
        EXPECT_CALL(sysMock, close(connFd)).WillOnce(Return(0));
        EXPECT_CALL(sysMock,
                    connect(connFd, reinterpret_cast<struct sockaddr*>(&sa),
                            sizeof(sa)))
            .WillOnce(Return(0));
    }

    internal::InternalSysMock sysMock;
    ipmiblob::BlobInterfaceMock blobMock;
    ProgressMock progMock;

    const std::string host = "::1"s;
    const std::string port = "622"s;

    struct sockaddr_in6 sa;
    struct addrinfo addr;

    static constexpr std::uint16_t session = 0xbeef;
    const std::string filePath = "/asdf"s;
    static constexpr int inFd = 5;
    static constexpr int connFd = 7;
    static constexpr size_t fakeFileSize = 128;
    static constexpr size_t chunkSize = 16;

    NetDataHandler handler;
};

TEST_F(NetHandleTest, openFileFail)
{
    EXPECT_CALL(sysMock, open(StrEq(filePath.c_str()), _))
        .WillOnce(SetErrnoAndReturn(EACCES, -1));

    EXPECT_FALSE(handler.sendContents(filePath, session));
}

TEST_F(NetHandleTest, getSizeFail)
{
    EXPECT_CALL(sysMock, open(StrEq(filePath.c_str()), _))
        .WillOnce(Return(inFd));
    EXPECT_CALL(sysMock, close(inFd)).WillOnce(Return(0));
    EXPECT_CALL(sysMock, getSize(StrEq(filePath.c_str()))).WillOnce(Return(0));

    EXPECT_FALSE(handler.sendContents(filePath, session));
}

TEST_F(NetHandleTest, getaddrinfoFail)
{
    expectOpenFile();

    EXPECT_CALL(sysMock,
                getaddrinfo(StrEq(host), StrEq(port),
                            AllOf(Field(&addrinfo::ai_flags, AI_NUMERICHOST),
                                  Field(&addrinfo::ai_family, AF_UNSPEC),
                                  Field(&addrinfo::ai_socktype, SOCK_STREAM)),
                            NotNull()))
        .WillOnce(Return(EAI_ADDRFAMILY));

    EXPECT_FALSE(handler.sendContents(filePath, session));
}

TEST_F(NetHandleTest, connectFail)
{
    expectOpenFile();
    expectAddrInfo();

    EXPECT_CALL(sysMock, socket(AF_INET6, SOCK_STREAM, 0))
        .WillOnce(Return(connFd));
    EXPECT_CALL(sysMock, close(connFd)).WillOnce(Return(0));
    EXPECT_CALL(
        sysMock,
        connect(connFd, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa)))
        .WillOnce(SetErrnoAndReturn(ECONNREFUSED, -1));

    EXPECT_FALSE(handler.sendContents(filePath, session));
}

TEST_F(NetHandleTest, sendfileFail)
{
    expectOpenFile();
    expectAddrInfo();
    expectConnection();

    EXPECT_CALL(sysMock, sendfile(connFd, inFd, Pointee(0), _))
        .WillOnce(SetErrnoAndReturn(ETIMEDOUT, -1));

    EXPECT_FALSE(handler.sendContents(filePath, session));
}

TEST_F(NetHandleTest, successOneChunk)
{
    expectOpenFile();
    expectAddrInfo();
    expectConnection();

    {
        InSequence seq;

        EXPECT_CALL(sysMock,
                    sendfile(connFd, inFd, Pointee(0), Gt(fakeFileSize)))
            .WillOnce(
                DoAll(SetArgPointee<2>(fakeFileSize), Return(fakeFileSize)));
        EXPECT_CALL(sysMock, sendfile(connFd, inFd, Pointee(fakeFileSize),
                                      Gt(fakeFileSize)))
            .WillOnce(Return(0));
    }

    struct ipmi_flash::ExtChunkHdr chunk;
    chunk.length = fakeFileSize;
    std::vector<std::uint8_t> chunkBytes(sizeof(chunk));
    std::memcpy(chunkBytes.data(), &chunk, sizeof(chunk));
    EXPECT_CALL(blobMock, writeBytes(session, 0, ContainerEq(chunkBytes)));

    EXPECT_CALL(progMock, updateProgress(fakeFileSize));

    EXPECT_TRUE(handler.sendContents(filePath, session));
}

TEST_F(NetHandleTest, successMultiChunk)
{
    expectOpenFile();
    expectAddrInfo();
    expectConnection();

    struct ipmi_flash::ExtChunkHdr chunk;
    chunk.length = chunkSize;
    std::vector<std::uint8_t> chunkBytes(sizeof(chunk));
    std::memcpy(chunkBytes.data(), &chunk, sizeof(chunk));

    {
        InSequence seq;

        for (std::uint32_t offset = 0; offset < fakeFileSize;
             offset += chunkSize)
        {
            EXPECT_CALL(sysMock,
                        sendfile(connFd, inFd, Pointee(offset), Gt(chunkSize)))
                .WillOnce(DoAll(SetArgPointee<2>(offset + chunkSize),
                                Return(chunkSize)));

            EXPECT_CALL(blobMock,
                        writeBytes(session, offset, ContainerEq(chunkBytes)));
            EXPECT_CALL(progMock, updateProgress(chunkSize));
        }
        EXPECT_CALL(sysMock, sendfile(connFd, inFd, Pointee(fakeFileSize),
                                      Gt(chunkSize)))
            .WillOnce(Return(0));
    }

    EXPECT_TRUE(handler.sendContents(filePath, session));
}

} // namespace
} // namespace host_tool
