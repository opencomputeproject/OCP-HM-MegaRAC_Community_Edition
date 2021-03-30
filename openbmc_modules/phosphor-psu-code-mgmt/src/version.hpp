#pragma once

#include "config.h"

#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Object/Delete/server.hpp>
#include <xyz/openbmc_project/Software/Version/server.hpp>

namespace phosphor
{
namespace software
{
namespace updater
{

using eraseFunc = std::function<void(std::string)>;

using VersionInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Software::server::Version>;
using DeleteInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Object::server::Delete>;

namespace sdbusRule = sdbusplus::bus::match::rules;

class Version;

/** @class Delete
 *  @brief OpenBMC Delete implementation.
 *  @details A concrete implementation for xyz.openbmc_project.Object.Delete
 *  D-Bus API.
 */
class Delete : public DeleteInherit
{
  public:
    /** @brief Constructs Delete.
     *
     *  @param[in] bus    - The D-Bus bus object
     *  @param[in] path   - The D-Bus object path
     *  @param[in] version - The Version object.
     */
    Delete(sdbusplus::bus::bus& bus, const std::string& path,
           Version& version) :
        DeleteInherit(bus, path.c_str(), action::emit_interface_added),
        version(version)
    {
    }

    /**
     * @brief Delete the D-Bus object.
     *        Overrides the default delete function by calling
     *        Version class erase Method.
     **/
    void delete_() override;

  private:
    /** @brief The Version Object. */
    Version& version;
};

/** @class Version
 *  @brief OpenBMC version software management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.Software.Version
 *  D-Bus API.
 */
class Version : public VersionInherit
{
  public:
    /** @brief Constructs Version Software Manager.
     *
     * @param[in] bus            - The D-Bus bus object
     * @param[in] objPath        - The D-Bus object path
     * @param[in] versionId      - The version Id
     * @param[in] versionString  - The version string
     * @param[in] versionPurpose - The version purpose
     * @param[in] callback       - The eraseFunc callback
     */
    Version(sdbusplus::bus::bus& bus, const std::string& objPath,
            const std::string& versionId, const std::string& versionString,
            VersionPurpose versionPurpose, eraseFunc callback) :
        VersionInherit(bus, (objPath).c_str(), true),
        eraseCallback(callback), bus(bus), objPath(objPath),
        versionId(versionId), versionStr(versionString)
    {
        // Set properties.
        purpose(versionPurpose);
        version(versionString);

        deleteObject = std::make_unique<Delete>(bus, objPath, *this);

        // Emit deferred signal.
        emit_object_added();
    }

    /**
     * @brief Return the version id
     */
    std::string getVersionId() const
    {
        return versionId;
    }

    /**
     * @brief Read the manifest file to get the values of the keys.
     *
     * @param[in] filePath - The path to the file which contains the value
     *                       of keys.
     * @param[in] keys     - A vector of keys.
     *
     * @return The map of keys with filled values.
     **/
    static std::map<std::string, std::string>
        getValues(const std::string& filePath,
                  const std::vector<std::string>& keys);

    /**
     * @brief Read the manifest file to get the value of the key.
     *
     * @param[in] filePath - The path to the file which contains the value
     *                       of keys.
     * @param[in] key      - The string of the key.
     *
     * @return The string of the value.
     **/
    static std::string getValue(const std::string& filePath,
                                const std::string& key);

    /** @brief Get information from extVersion
     *
     * @param[in] extVersion - The extended version string that contains
     *                         key/value pairs separated by comma.
     *
     * @return The map of key/value pairs
     */
    static std::map<std::string, std::string>
        getExtVersionInfo(const std::string& extVersion);

    /** @brief The temUpdater's erase callback. */
    eraseFunc eraseCallback;

    /** @brief Get the version string. */
    const std::string& getVersionString() const
    {
        return versionStr;
    }

  private:
    /** @brief Persistent sdbusplus DBus bus connection */
    sdbusplus::bus::bus& bus;

    /** @brief Persistent DBus object path */
    std::string objPath;

    /** @brief This Version's version Id */
    const std::string versionId;

    /** @brief This Version's version string */
    const std::string versionStr;

    /** @brief Persistent Delete D-Bus object */
    std::unique_ptr<Delete> deleteObject;
};

} // namespace updater
} // namespace software
} // namespace phosphor
