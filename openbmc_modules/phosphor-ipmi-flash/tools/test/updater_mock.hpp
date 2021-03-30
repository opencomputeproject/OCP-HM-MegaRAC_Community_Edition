#pragma once

#include "updater.hpp"

#include <string>

#include <gmock/gmock.h>

namespace host_tool
{

class UpdateHandlerMock : public UpdateHandlerInterface
{
  public:
    MOCK_METHOD1(checkAvailable, bool(const std::string&));
    MOCK_METHOD2(sendFile, void(const std::string&, const std::string&));
    MOCK_METHOD2(verifyFile, bool(const std::string&, bool));
    MOCK_METHOD0(cleanArtifacts, void());
};

} // namespace host_tool
