#pragma once

#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace phosphor
{
namespace led
{

namespace fs = std::filesystem;

// the set of names of asserted groups, which contains the D-Bus Object path
using SavedGroups = std::set<std::string>;

/** @class Serialize
 *  @brief Store and restore groups of LEDs
 */
class Serialize
{
  public:
    Serialize(const fs::path& path) : path(path)
    {
        restoreGroups();
    }

    /** @brief Store asserted group names to SAVED_GROUPS_FILE
     *
     *  @param [in] group     - name of the group
     *  @param [in] asserted  - asserted state, true or false
     */
    void storeGroups(const std::string& group, bool asserted);

    /** @brief Is the group in asserted state stored in SAVED_GROUPS_FILE
     *
     *  @param [in] objPath - The D-Bus path that hosts LED group
     *
     *  @return             - true: exist, false: does not exist
     */
    bool getGroupSavedState(const std::string& objPath) const;

  private:
    /** @brief restore asserted group names from SAVED_GROUPS_FILE
     */
    void restoreGroups();

    /** @brief the set of names of asserted groups */
    SavedGroups savedGroups;

    /** @brief the path of file for storing the names of asserted groups */
    fs::path path;
};

} // namespace led
} // namespace phosphor
