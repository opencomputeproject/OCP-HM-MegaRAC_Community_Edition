/* Interface that implements an exception throwing write method. */
#pragma once

#include "interfaces.hpp"

class WriteOnly : public ReadInterface
{
  public:
    WriteOnly() : ReadInterface()
    {
    }

    ReadReturn read(void) override;
};
