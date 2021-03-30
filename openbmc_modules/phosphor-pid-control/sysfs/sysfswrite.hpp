#pragma once

#include "interfaces.hpp"
#include "util.hpp"

#include <string>

/*
 * A WriteInterface that is expecting a path that's sysfs, but really could be
 * any filesystem path.
 */
class SysFsWritePercent : public WriteInterface
{
  public:
    SysFsWritePercent(const std::string& writePath, int64_t min, int64_t max) :
        WriteInterface(min, max), _writePath(FixupPath(writePath))
    {
    }

    void write(double value) override;

  private:
    std::string _writePath;
};

class SysFsWrite : public WriteInterface
{
  public:
    SysFsWrite(const std::string& writePath, int64_t min, int64_t max) :
        WriteInterface(min, max), _writePath(FixupPath(writePath))
    {
    }

    void write(double value) override;

  private:
    std::string _writePath;
};
