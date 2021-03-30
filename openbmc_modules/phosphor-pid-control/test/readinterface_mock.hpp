#pragma once

#include "interfaces.hpp"

#include <gmock/gmock.h>

class ReadInterfaceMock : public ReadInterface
{
  public:
    virtual ~ReadInterfaceMock() = default;

    MOCK_METHOD0(read, ReadReturn());
};
