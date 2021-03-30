#pragma once

#include "fs.hpp"

#include <string>

#include <gmock/gmock.h>

namespace ipmi_flash
{

class FileSystemMock : public FileSystemInterface
{
  public:
    MOCK_CONST_METHOD1(remove, void(const std::string&));
};
} // namespace ipmi_flash
