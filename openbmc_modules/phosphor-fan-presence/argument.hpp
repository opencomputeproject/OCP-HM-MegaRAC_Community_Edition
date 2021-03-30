#pragma once

#include <getopt.h>

#include <map>
#include <string>

namespace phosphor
{
namespace fan
{
namespace util
{

/**
 * Parses command line arguments.
 * This header can be used by all fan applications, while
 * the .cpp will need to be implemented by each of them.
 */
class ArgumentParser
{
  public:
    ArgumentParser(int argc, char** argv);
    ArgumentParser() = delete;
    ArgumentParser(const ArgumentParser&) = delete;
    ArgumentParser(ArgumentParser&&) = default;
    ArgumentParser& operator=(const ArgumentParser&) = delete;
    ArgumentParser& operator=(ArgumentParser&&) = default;
    ~ArgumentParser() = default;
    const std::string& operator[](const std::string& opt);

    static void usage(char** argv);

    static const std::string true_string;
    static const std::string empty_string;

  private:
    std::map<const std::string, std::string> arguments;

    static const option options[];
    static const char* optionstr;
};

} // namespace util
} // namespace fan
} // namespace phosphor
