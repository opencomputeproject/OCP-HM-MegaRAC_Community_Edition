#pragma once

#include "sys.hpp"

#include <gmock/gmock.h>

namespace binstore
{
namespace internal
{

class SysMock : public internal::Sys
{
  public:
    MOCK_CONST_METHOD2(open, int(const char*, int));
    MOCK_CONST_METHOD1(close, int(int));
    MOCK_CONST_METHOD3(lseek, off_t(int, off_t, int));
    MOCK_CONST_METHOD3(read, ssize_t(int, void*, size_t));
    MOCK_CONST_METHOD3(write, ssize_t(int, const void*, size_t));
};

} // namespace internal

} // namespace binstore
