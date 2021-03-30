#pragma once

#include "io.hpp"

#include <gmock/gmock.h>

namespace host_tool
{

class HostIoInterfaceMock : public HostIoInterface
{
  public:
    ~HostIoInterfaceMock() = default;

    MOCK_METHOD3(read, bool(const std::size_t, const std::size_t, void* const));

    MOCK_METHOD3(write,
                 bool(const std::size_t, const std::size_t, const void* const));
};

} // namespace host_tool
