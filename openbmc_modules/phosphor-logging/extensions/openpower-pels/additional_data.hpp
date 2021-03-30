#pragma once
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace openpower
{
namespace pels
{

/**
 * @class AdditionalData
 *
 * This class takes in the contents of the AdditionalData OpenBMC
 * event log property, and provides access to its values based on
 * their keys.
 *
 * The property is a vector of strings of the form: "KEY=VALUE",
 * and this class provides a getValue("KEY") API that would return
 * "VALUE".
 */
class AdditionalData
{
  public:
    AdditionalData() = default;
    ~AdditionalData() = default;
    AdditionalData(const AdditionalData&) = default;
    AdditionalData& operator=(const AdditionalData&) = default;
    AdditionalData(AdditionalData&&) = default;
    AdditionalData& operator=(AdditionalData&&) = default;

    /**
     * @brief constructor
     *
     * @param[in] ad - the AdditionalData property vector with
     *                 entries of "KEY=VALUE"
     */
    explicit AdditionalData(const std::vector<std::string>& ad)
    {
        for (auto& item : ad)
        {
            auto pos = item.find_first_of('=');
            if (pos == std::string::npos || pos == 0)
            {
                continue;
            }

            _data[item.substr(0, pos)] = std::move(item.substr(pos + 1));
        }
    }

    /**
     * @brief Returns the value of the AdditionalData item for the
     *        key passed in.
     * @param[in] key - the key to search for
     *
     * @return optional<string> - the value, if found
     */
    std::optional<std::string> getValue(const std::string& key) const
    {
        auto entry = _data.find(key);
        if (entry != _data.end())
        {
            return entry->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Remove a key/value pair from the contained data
     *
     * @param[in] key - The key of the entry to remove
     */
    void remove(const std::string& key)
    {
        _data.erase(key);
    }

    /**
     * @brief Says if the object has no data
     *
     * @return bool true if the object is empty
     */
    inline bool empty() const
    {
        return _data.empty();
    }

    /**
     * @brief Returns the contained data as a JSON object
     *
     * Looks like: {"key1":"value1","key2":"value2"}
     *
     * @return json - The JSON object
     */
    nlohmann::json toJSON() const
    {
        nlohmann::json j = _data;
        return j;
    }

    /**
     * @brief Returns the underlying map of data
     *
     * @return const std::map<std::string, std::string>& - The data
     */
    const std::map<std::string, std::string>& getData() const
    {
        return _data;
    }

    /**
     * @brief Adds a key/value pair to the object
     *
     * @param[in] key - The key
     * @param[in] value - The value
     */
    void add(const std::string& key, const std::string& value)
    {
        _data.emplace(key, value);
    }

  private:
    /**
     * @brief a map of keys to values
     */
    std::map<std::string, std::string> _data;
};
} // namespace pels
} // namespace openpower
