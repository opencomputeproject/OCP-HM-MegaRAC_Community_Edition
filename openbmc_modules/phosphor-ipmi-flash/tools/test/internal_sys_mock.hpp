#pragma once

#include "internal/sys.hpp"

#include <unistd.h>

#include <cstdint>

#include <gmock/gmock.h>

namespace internal
{

class InternalSysMock : public Sys
{
  public:
    virtual ~InternalSysMock() = default;

    MOCK_CONST_METHOD2(open, int(const char*, int));
    MOCK_CONST_METHOD3(read, int(int, void*, std::size_t));
    MOCK_CONST_METHOD4(pread, int(int, void*, std::size_t, off_t));
    MOCK_CONST_METHOD4(pwrite, int(int, const void*, std::size_t, off_t));
    MOCK_CONST_METHOD1(close, int(int));
    MOCK_CONST_METHOD6(mmap, void*(void*, std::size_t, int, int, int, off_t));
    MOCK_CONST_METHOD2(munmap, int(void*, std::size_t));
    MOCK_CONST_METHOD0(getpagesize, int());
    MOCK_CONST_METHOD3(ioctl, int(int, unsigned long, void*));
    MOCK_CONST_METHOD3(poll, int(struct pollfd*, nfds_t, int));
    MOCK_CONST_METHOD3(socket, int(int, int, int));
    MOCK_CONST_METHOD3(connect, int(int, const struct sockaddr*, socklen_t));
    MOCK_CONST_METHOD4(sendfile, ssize_t(int, int, off_t*, size_t));
    MOCK_CONST_METHOD4(getaddrinfo,
                       int(const char*, const char*, const struct addrinfo*,
                           struct addrinfo**));
    MOCK_CONST_METHOD1(freeaddrinfo, void(struct addrinfo*));
    MOCK_CONST_METHOD1(getSize, std::int64_t(const char*));
};

} // namespace internal
