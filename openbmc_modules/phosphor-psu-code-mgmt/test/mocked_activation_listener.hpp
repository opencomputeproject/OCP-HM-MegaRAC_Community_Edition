#pragma once

#include "activation_listener.hpp"

#include <gmock/gmock.h>

class MockedActivationListener : public ActivationListener
{
  public:
    virtual ~MockedActivationListener() = default;

    MOCK_METHOD2(onUpdateDone, void(const std::string& versionId,
                                    const std::string& psuInventoryPath));
};
