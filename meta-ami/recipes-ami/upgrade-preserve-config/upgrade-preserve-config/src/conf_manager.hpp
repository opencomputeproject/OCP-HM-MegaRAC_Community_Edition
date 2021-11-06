#pragma once

#include "preserve.hpp"

#include <sdbusplus/bus.hpp>

#include <experimental/filesystem>

namespace phosphor
{
namespace software
{
namespace preserve
{

namespace fs = std::experimental::filesystem;

class ConfManager
{
  public:
    ConfManager() = delete;
    ConfManager(const ConfManager&) = delete;
    ConfManager& operator=(const ConfManager&) = delete;
    ConfManager(ConfManager&&) = delete;
    ConfManager& operator=(ConfManager&&) = delete;
    virtual ~ConfManager() = default;

    /** @brief Constructor to put object onto bus at a D-Bus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     */
    ConfManager(sdbusplus::bus::bus& bus, const char* objPath);

    /** @brief Restore the preservation configuration from its persisted
     *         representation.
     */
    void restoreConfig();

    /** @brief location of the persisted D-Bus object.*/
    fs::path dbusPersistentLocation;

  private:
    /** @brief sdbusplus DBus bus object. */
    sdbusplus::bus::bus& bus;

    /** @brief Path of Object. */
    std::string objectPath;

    /* @brief preservation configuration object */
    std::unique_ptr<PreserveConf> preserve;
};

} // namespace preserve
} // namespace software
} // namespace phosphor
