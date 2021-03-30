#pragma once
#include "property_change_listener.hpp"

#include <gmock/gmock.h>

namespace phosphor
{
namespace time
{

class MockPropertyChangeListner : public PropertyChangeListner
{
  public:
    MOCK_METHOD1(onModeChanged, void(Mode mode));
};

} // namespace time
} // namespace phosphor
