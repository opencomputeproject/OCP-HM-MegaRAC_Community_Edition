#pragma once

#include <string>

namespace ipmi_flash
{

class FileSystemInterface
{
  public:
    virtual ~FileSystemInterface() = default;

    virtual void remove(const std::string& path) const = 0;
};

class FileSystem : public FileSystemInterface
{
  public:
    FileSystem() = default;

    void remove(const std::string& path) const override;
};

} // namespace ipmi_flash
