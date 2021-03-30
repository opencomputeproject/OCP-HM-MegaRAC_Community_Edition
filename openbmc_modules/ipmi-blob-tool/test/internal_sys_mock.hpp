#pragma once

#include <unistd.h>

#include <ipmiblob/internal/sys.hpp>

#include <gmock/gmock.h>

namespace ipmiblob
{
namespace internal
{

class InternalSysMock : public Sys
{
  public:
    virtual ~InternalSysMock() = default;

    MOCK_CONST_METHOD2(open, int(const char*, int));
    MOCK_CONST_METHOD3(read, int(int, void*, std::size_t));
    MOCK_CONST_METHOD1(close, int(int));
    MOCK_CONST_METHOD6(mmap, void*(void*, std::size_t, int, int, int, off_t));
    MOCK_CONST_METHOD2(munmap, int(void*, std::size_t));
    MOCK_CONST_METHOD0(getpagesize, int());
    MOCK_CONST_METHOD3(ioctl, int(int, unsigned long, void*));
    MOCK_CONST_METHOD3(poll, int(struct pollfd*, nfds_t, int));
};

} // namespace internal
} // namespace ipmiblob
