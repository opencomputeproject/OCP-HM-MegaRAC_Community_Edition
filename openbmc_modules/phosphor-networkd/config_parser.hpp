#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace phosphor
{
namespace network
{
namespace config
{

using Section = std::string;
using KeyValueMap = std::multimap<std::string, std::string>;
using ValueList = std::vector<std::string>;

namespace fs = std::filesystem;

enum class ReturnCode
{
    SUCCESS = 0x0,
    SECTION_NOT_FOUND = 0x1,
    KEY_NOT_FOUND = 0x2,
};

class Parser
{
  public:
    Parser() = default;

    /** @brief Constructor
     *  @param[in] fileName - Absolute path of the file which will be parsed.
     */

    Parser(const fs::path& fileName);

    /** @brief Get the values of the given key and section.
     *  @param[in] section - section name.
     *  @param[in] key - key to look for.
     *  @returns the tuple of return code and the
     *           values associated with the key.
     */

    std::tuple<ReturnCode, ValueList> getValues(const std::string& section,
                                                const std::string& key);

    /** @brief Set the value of the given key and section.
     *  @param[in] section - section name.
     *  @param[in] key - key name.
     *  @param[in] value - value.
     */

    void setValue(const std::string& section, const std::string& key,
                  const std::string& value);

    /** @brief Set the file name and parse it.
     *  @param[in] fileName - Absolute path of the file.
     */

    void setFile(const fs::path& fileName);

  private:
    /** @brief Parses the given file and fills the data.
     *  @param[in] stream - inputstream.
     */

    void parse(std::istream& stream);

    /** @brief Get all the key values of the given section.
     *  @param[in] section - section name.
     *  @returns the tuple of return code and the map of (key,value).
     */

    std::tuple<ReturnCode, KeyValueMap> getSection(const std::string& section);

    /** @brief checks that whether the value exist in the
     *         given section.
     *  @param[in] section - section name.
     *  @param[in] key - key name.
     *  @param[in] value - value.
     *  @returns true if exist otherwise false.
     */

    bool isValueExist(const std::string& section, const std::string& key,
                      const std::string& value);

    std::unordered_map<Section, KeyValueMap> sections;
    fs::path filePath;
};

} // namespace config
} // namespace network
} // namespace phosphor
