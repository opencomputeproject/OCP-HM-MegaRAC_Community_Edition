#pragma once

#include <exception>
#include <string>

namespace ipmiblob
{

class BlobException : public std::exception
{
  public:
    explicit BlobException(const std::string& message) : message(message){};

    virtual const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};

} // namespace ipmiblob
