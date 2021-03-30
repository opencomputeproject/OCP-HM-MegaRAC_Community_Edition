#pragma once

/* Interface that implements an exception throwing read method. */

#include "interfaces.hpp"

class ReadOnly : public WriteInterface
{
  public:
    ReadOnly() : WriteInterface(0, 0)
    {
    }

    void write(double value) override;
};

class ReadOnlyNoExcept : public WriteInterface
{
  public:
    ReadOnlyNoExcept() : WriteInterface(0, 0)
    {
    }

    void write(double value) override;
};
