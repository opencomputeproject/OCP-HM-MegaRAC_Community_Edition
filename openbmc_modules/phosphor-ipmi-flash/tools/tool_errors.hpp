#pragma once

#include <exception>
#include <string>

namespace host_tool
{

class ToolException : public std::exception
{
  public:
    explicit ToolException(const std::string& message) : message(message){};

    virtual const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};

class NotFoundException : public ToolException
{
  public:
    explicit NotFoundException(const std::string& device) :
        ToolException(std::string("Couldn't find " + device)){};
};

} // namespace host_tool
