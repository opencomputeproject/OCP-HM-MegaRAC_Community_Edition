#pragma once

#include <ipmiblob/ipmi_interface.hpp>

#include <gmock/gmock.h>

namespace ipmiblob
{

class IpmiInterfaceMock : public IpmiInterface
{
  public:
    virtual ~IpmiInterfaceMock() = default;
    MOCK_METHOD3(sendPacket,
                 std::vector<std::uint8_t>(std::uint8_t, std::uint8_t,
                                           std::vector<std::uint8_t>&));
};

} // namespace ipmiblob
