#pragma once
#include <gmock/gmock.h>
#include <gpioplus/event.hpp>

namespace gpioplus
{
namespace test
{

class EventMock : public EventInterface
{
  public:
    MOCK_CONST_METHOD0(read, std::optional<Data>());
    MOCK_CONST_METHOD0(getValue, uint8_t());
};

} // namespace test
} // namespace gpioplus
