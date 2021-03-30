#pragma once

#include <map>
#include <set>
#include <string>

/**
 * @class SensorSet
 * @brief Finds and holds the available hwmon sensors for a device
 * @details When passed a hwmon device directory on construction,
 *          this class will find all hwmon sysfs files in that directory
 *          and store them in a map.  The public begin() and end() methods
 *          on this class allow traversal of this map.
 *
 *          For example, a file named temp5_input will have a map entry of:
 *
 *              key:   pair<string, string> = {"temp", "5"}
 *              value: std::string = "input"
 */
class SensorSet
{
  public:
    typedef std::map<std::pair<std::string, std::string>, std::set<std::string>>
        container_t;
    using mapped_type = container_t::mapped_type;
    using key_type = container_t::key_type;

    /**
     * @brief Constructor
     * @details Builds a map of the hwmon sysfs files in the passed
     *          in directory.
     *
     * @param[in] path - path to the hwmon device directory
     *
     */
    explicit SensorSet(const std::string& path);
    ~SensorSet() = default;
    SensorSet() = delete;
    SensorSet(const SensorSet&) = delete;
    SensorSet& operator=(const SensorSet&) = delete;
    SensorSet(SensorSet&&) = default;
    SensorSet& operator=(SensorSet&&) = default;

    /**
     * @brief Returns an iterator to the beginning of the map
     *
     * @return const_iterator
     */
    container_t::const_iterator begin()
    {
        return const_cast<const container_t&>(_container).begin();
    }

    /**
     * @brief Returns an iterator to the end of the map
     *
     * @return const_iterator
     */
    container_t::const_iterator end()
    {
        return const_cast<const container_t&>(_container).end();
    }

  private:
    /**
     * @brief The map of hwmon files in the directory
     * @details For example:
     *          key = pair("temp", "1")
     *          value = "input"
     */
    container_t _container;
};

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
