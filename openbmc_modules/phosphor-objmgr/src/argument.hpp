#pragma once

#include <getopt.h>

#include <map>
#include <string>

/** @brief Class - Encapsulates parsing command line options and
 *                 populating arguments
 */
class ArgumentParser
{
  public:
    ArgumentParser() = delete;
    ~ArgumentParser() = default;
    ArgumentParser(const ArgumentParser&) = delete;
    ArgumentParser& operator=(const ArgumentParser&) = delete;
    ArgumentParser(ArgumentParser&&) = default;
    ArgumentParser& operator=(ArgumentParser&&) = default;

    /** @brief Constructs Argument object
     *
     *  @param argc - the main function's argc passed as is
     *  @param argv - the main function's argv passed as is
     *  @return Object constructed
     */
    ArgumentParser(int argc, char** argv);

    /** @brief Given a option, returns its argument(optarg) */
    const std::string& operator[](const std::string& opt);

    /** @brief Displays usage */
    static void usage(char** argv);

    /** @brief Set to 'true' when an option is passed */
    static const std::string true_string;

    /** @brief Set to '' when an option is not passed */
    static const std::string empty_string;

  private:
    /** @brief Option to argument mapping */
    std::map<const std::string, std::string> arguments;

    /** @brief Array of struct options as needed by getopt_long */
    static const option options[];

    /** @brief optstring as needed by getopt_long */
    static const char* optionstr;
};
