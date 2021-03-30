#pragma once

#include "hwmonio.hpp"

#include <string>

#include <gmock/gmock.h>

namespace hwmonio
{

class FileSystemMock : public FileSystemInterface
{
  public:
    MOCK_CONST_METHOD1(read, int64_t(const std::string&));
    MOCK_CONST_METHOD2(write, void(const std::string&, uint32_t));
};

} // namespace hwmonio
