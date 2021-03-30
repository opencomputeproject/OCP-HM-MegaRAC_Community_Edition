#pragma once

#include <exception>
#include <string>

namespace ipmi_flash
{

class MapperException : public std::exception
{
  public:
    explicit MapperException(const std::string& message) : message(message)
    {}

    virtual const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};

} // namespace ipmi_flash
