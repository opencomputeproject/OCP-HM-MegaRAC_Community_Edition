#pragma once

#include "progress.hpp"

#include <cstdint>

#include <gmock/gmock.h>

namespace host_tool
{

class ProgressMock : public ProgressInterface
{
  public:
    MOCK_METHOD1(updateProgress, void(std::int64_t));
    MOCK_METHOD1(start, void(std::int64_t));
};

} // namespace host_tool
