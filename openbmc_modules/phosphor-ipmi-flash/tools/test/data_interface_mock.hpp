#pragma once

#include "interface.hpp"

#include <gmock/gmock.h>

namespace host_tool
{

class DataInterfaceMock : public DataInterface
{

  public:
    virtual ~DataInterfaceMock() = default;

    MOCK_METHOD2(sendContents, bool(const std::string&, std::uint16_t));
    MOCK_CONST_METHOD0(supportedType, ipmi_flash::FirmwareFlags::UpdateFlags());
};

} // namespace host_tool
