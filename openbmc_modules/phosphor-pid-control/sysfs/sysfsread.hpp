#pragma once

#include "interfaces.hpp"
#include "util.hpp"

#include <string>

/*
 * A ReadInterface that is expecting a path that's sysfs, but really could be
 * any filesystem path.
 */
class SysFsRead : public ReadInterface
{
  public:
    SysFsRead(const std::string& path) : ReadInterface(), _path(FixupPath(path))
    {
    }

    ReadReturn read(void) override;

  private:
    const std::string _path;
};
