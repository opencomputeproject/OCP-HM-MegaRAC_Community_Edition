#pragma once

#include <exception>
#include <string>

class SensorBuildException : public std::exception
{
  public:
    SensorBuildException(const std::string& message) : message(message)
    {
    }

    virtual const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};

class ControllerBuildException : public std::exception
{
  public:
    ControllerBuildException(const std::string& message) : message(message)
    {
    }

    virtual const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};

class ConfigurationException : public std::exception
{
  public:
    ConfigurationException(const std::string& message) : message(message)
    {
    }

    virtual const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};
