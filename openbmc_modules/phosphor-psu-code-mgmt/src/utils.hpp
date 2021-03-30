#pragma once

#include "types.hpp"

#include <experimental/any>
#include <sdbusplus/bus.hpp>
#include <set>
#include <string>
#include <vector>

namespace utils
{

class UtilsInterface;

using AssociationList = phosphor::software::updater::AssociationList;
// Due to a libstdc++ bug, we got compile error using std::any with gmock.
// A temporary workaround is to use std::experimental::any.
// See details in https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90415
using std::experimental::any;
using std::experimental::any_cast;

/**
 * @brief Get the implementation of UtilsInterface
 */
const UtilsInterface& getUtils();

/**
 * @brief Get PSU inventory object path from DBus
 */
std::vector<std::string> getPSUInventoryPath(sdbusplus::bus::bus& bus);

/** @brief Get service name from object path and interface
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] path         - The Dbus object path
 * @param[in] interface    - The Dbus interface
 *
 * @return The name of the service
 */
std::string getService(sdbusplus::bus::bus& bus, const char* path,
                       const char* interface);

/** @brief Get all the service names from object path and interface
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] path         - The Dbus object path
 * @param[in] interface    - The Dbus interface
 *
 * @return The name of the services
 */
std::vector<std::string> getServices(sdbusplus::bus::bus& bus, const char* path,
                                     const char* interface);

/** @brief The template function to get property from the requested dbus path
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] service      - The Dbus service name
 * @param[in] path         - The Dbus object path
 * @param[in] interface    - The Dbus interface
 * @param[in] propertyName - The property name to get
 *
 * @return The value of the property
 */
template <typename T>
T getProperty(sdbusplus::bus::bus& bus, const char* service, const char* path,
              const char* interface, const char* propertyName);

/**
 * @brief Calculate the version id from the version string.
 *
 * @details The version id is a unique 8 hexadecimal digit id
 *          calculated from the version string.
 *
 * @param[in] version - The image version string (e.g. v1.99.10-19).
 *
 * @return The id.
 */
std::string getVersionId(const std::string& version);

/** @brief Get version of PSU specified by the inventory path
 *
 * @param[in] inventoryPath - The PSU inventory object path
 *
 * @return The version string, or empry string if it fails to get the version
 */
std::string getVersion(const std::string& inventoryPath);

/** @brief Get latest version from the PSU versions
 *
 * @param[in] versions - The list of the versions
 *
 * @return The latest version string
 */
std::string getLatestVersion(const std::set<std::string>& versions);

/** @brief Check if the PSU is associated
 *
 * @param[in] psuInventoryPath - The PSU inventory path
 * @param[in] assocs - The list of associations
 *
 * @return true if the psu is in the association list
 */
bool isAssociated(const std::string& psuInventoryPath,
                  const AssociationList& assocs);

/**
 * @brief The interface for utils
 */
class UtilsInterface
{
  public:
    // For now the code needs to get property for Present and Version
    using PropertyType = std::variant<std::string, bool>;

    virtual ~UtilsInterface() = default;

    virtual std::vector<std::string>
        getPSUInventoryPath(sdbusplus::bus::bus& bus) const = 0;

    virtual std::string getService(sdbusplus::bus::bus& bus, const char* path,
                                   const char* interface) const = 0;

    virtual std::vector<std::string>
        getServices(sdbusplus::bus::bus& bus, const char* path,
                    const char* interface) const = 0;

    virtual std::string getVersionId(const std::string& version) const = 0;

    virtual std::string getVersion(const std::string& inventoryPath) const = 0;

    virtual std::string
        getLatestVersion(const std::set<std::string>& versions) const = 0;

    virtual bool isAssociated(const std::string& psuInventoryPath,
                              const AssociationList& assocs) const = 0;

    virtual any getPropertyImpl(sdbusplus::bus::bus& bus, const char* service,
                                const char* path, const char* interface,
                                const char* propertyName) const = 0;

    template <typename T>
    T getProperty(sdbusplus::bus::bus& bus, const char* service,
                  const char* path, const char* interface,
                  const char* propertyName) const
    {
        any result =
            getPropertyImpl(bus, service, path, interface, propertyName);
        auto value = any_cast<PropertyType>(result);
        return std::get<T>(value);
    }
};

class Utils : public UtilsInterface
{
  public:
    std::vector<std::string>
        getPSUInventoryPath(sdbusplus::bus::bus& bus) const override;

    std::string getService(sdbusplus::bus::bus& bus, const char* path,
                           const char* interface) const override;

    std::vector<std::string> getServices(sdbusplus::bus::bus& bus,
                                         const char* path,
                                         const char* interface) const override;

    std::string getVersionId(const std::string& version) const override;

    std::string getVersion(const std::string& inventoryPath) const override;

    std::string
        getLatestVersion(const std::set<std::string>& versions) const override;

    bool isAssociated(const std::string& psuInventoryPath,
                      const AssociationList& assocs) const override;

    any getPropertyImpl(sdbusplus::bus::bus& bus, const char* service,
                        const char* path, const char* interface,
                        const char* propertyName) const override;
};

inline std::string getService(sdbusplus::bus::bus& bus, const char* path,
                              const char* interface)
{
    return getUtils().getService(bus, path, interface);
}

inline std::vector<std::string> getServices(sdbusplus::bus::bus& bus,
                                            const char* path,
                                            const char* interface)
{
    return getUtils().getServices(bus, path, interface);
}

inline std::vector<std::string> getPSUInventoryPath(sdbusplus::bus::bus& bus)
{
    return getUtils().getPSUInventoryPath(bus);
}

inline std::string getVersionId(const std::string& version)
{
    return getUtils().getVersionId(version);
}

inline std::string getVersion(const std::string& inventoryPath)
{
    return getUtils().getVersion(inventoryPath);
}

inline std::string getLatestVersion(const std::set<std::string>& versions)
{
    return getUtils().getLatestVersion(versions);
}

inline bool isAssociated(const std::string& psuInventoryPath,
                         const AssociationList& assocs)
{
    return getUtils().isAssociated(psuInventoryPath, assocs);
}

template <typename T>
T getProperty(sdbusplus::bus::bus& bus, const char* service, const char* path,
              const char* interface, const char* propertyName)
{
    return getUtils().getProperty<T>(bus, service, path, interface,
                                     propertyName);
}

} // namespace utils
