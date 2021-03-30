#pragma once

#include "handler.hpp"

#include <cstdint>
#include <string>

#include <gmock/gmock.h>

namespace ethstats
{

class HandlerMock : public EthStatsInterface
{
  public:
    ~HandlerMock() = default;

    MOCK_CONST_METHOD1(validIfNameAndField, bool(const std::string&));
    MOCK_CONST_METHOD1(readStatistic, std::uint64_t(const std::string&));
};

} // namespace ethstats
