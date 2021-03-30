#pragma once
#include "utility.hpp"

#include <libevdev/libevdev.h>

namespace phosphor
{
namespace cooling
{
namespace type
{

struct FreeEvDev
{
    void operator()(struct libevdev* device) const
    {
        libevdev_free(device);
    }
};

class CoolingType
{
    using Property = std::string;
    using Value = std::variant<bool>;
    // Association between property and its value
    using PropertyMap = std::map<Property, Value>;
    using Interface = std::string;
    // Association between interface and the dbus property
    using InterfaceMap = std::map<Interface, PropertyMap>;
    using Object = sdbusplus::message::object_path;
    // Association between object and the interface
    using ObjectMap = std::map<Object, InterfaceMap>;

  public:
    CoolingType() = delete;
    ~CoolingType() = default;
    CoolingType(const CoolingType&) = delete;
    CoolingType(CoolingType&&) = default;
    CoolingType& operator=(const CoolingType&) = delete;
    CoolingType& operator=(CoolingType&&) = default;

    /**
     * @brief Constructs Cooling Type Object
     *
     * @param[in] bus - Dbus bus object
     */
    explicit CoolingType(sdbusplus::bus::bus& bus) : bus(bus), gpioFd(-1)
    {
        // TODO: Issue openbmc/openbmc#1531 - means to default properties.
    }

    /**
     * @brief Sets airCooled to true.
     */
    void setAirCooled();
    /**
     * @brief Sets waterCooled to true.
     */
    void setWaterCooled();
    /**
     * @brief Updates the inventory properties for CoolingType.
     *
     * @param[in] path - Path to object to update
     */
    void updateInventory(const std::string& path);
    /**
     * @brief Setup and read the GPIO device for reading cooling type.
     *
     * @param[in] path - Path to the GPIO device file to read
     * @param[in] pin  - Event/key code to read (pin)
     */
    void readGpio(const std::string& path, unsigned int pin);

  private:
    /** @brief Connection for sdbusplus bus */
    sdbusplus::bus::bus& bus;
    // File descriptor for the GPIO file we are going to read.
    phosphor::fan::util::FileDescriptor gpioFd;
    bool airCooled = false;
    bool waterCooled = false;

    /**
     * @brief Construct the inventory object map for CoolingType.
     *
     * @param[in] objpath - Path to object to update
     *
     * @return The inventory object map to update inventory
     */
    ObjectMap getObjectMap(const std::string& objpath);
};

} // namespace type
} // namespace cooling
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
