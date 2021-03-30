#pragma once
#include "evdev.hpp"

#include <systemd/sd-event.h>

#include <experimental/filesystem>
#include <string>

namespace phosphor
{
namespace gpio
{
namespace presence
{

static constexpr auto deviceField = 0;
static constexpr auto pathField = 1;
using Device = std::string;
using Path = std::experimental::filesystem::path;
using Driver = std::tuple<Device, Path>;
using Interface = std::string;

/** @class Presence
 *  @brief Responsible for determining and monitoring presence,
 *  by monitoring GPIO state changes, of inventory items and
 *  updating D-Bus accordingly.
 */
class Presence : public Evdev
{

    using Property = std::string;
    using Value = std::variant<bool, std::string>;
    // Association between property and its value
    using PropertyMap = std::map<Property, Value>;
    using Interface = std::string;
    // Association between interface and the D-Bus property
    using InterfaceMap = std::map<Interface, PropertyMap>;
    using Object = sdbusplus::message::object_path;
    // Association between object and the interface
    using ObjectMap = std::map<Object, InterfaceMap>;

  public:
    Presence() = delete;
    ~Presence() = default;
    Presence(const Presence&) = delete;
    Presence& operator=(const Presence&) = delete;
    Presence(Presence&&) = delete;
    Presence& operator=(Presence&&) = delete;

    /** @brief Constructs Presence object.
     *
     *  @param[in] bus       - D-Bus bus Object
     *  @param[in] inventory - Object path under inventory
                               to display this inventory item
     *  @param[in] path      - Device path to read for GPIO pin state
                               to determine presence of inventory item
     *  @param[in] key       - GPIO key to monitor
     *  @param[in] name      - Pretty name of the inventory item
     *  @param[in] event     - sd_event handler
     *  @param[in] drivers   - list of device drivers to bind and unbind
     *  @param[in] ifaces    - list of extra interfaces to associate with the
     *                         inventory item
     *  @param[in] handler   - IO callback handler. Defaults to one in this
     *                        class
     */
    Presence(sdbusplus::bus::bus& bus, const std::string& inventory,
             const std::string& path, const unsigned int key,
             const std::string& name, EventPtr& event,
             const std::vector<Driver>& drivers,
             const std::vector<Interface>& ifaces,
             sd_event_io_handler_t handler = Presence::processEvents) :
        Evdev(path, key, event, handler, true),
        bus(bus), inventory(inventory), name(name), drivers(drivers),
        ifaces(ifaces)
    {
        determinePresence();
    }

    /** @brief Callback handler when the FD has some activity on it
     *
     *  @param[in] es       - Populated event source
     *  @param[in] fd       - Associated File descriptor
     *  @param[in] revents  - Type of event
     *  @param[in] userData - User data that was passed during registration
     *
     *  @return             - 0 or positive number on success and negative
     *                        errno otherwise
     */
    static int processEvents(sd_event_source* es, int fd, uint32_t revents,
                             void* userData);

  private:
    /**
     * @brief Update the present property for the inventory item.
     *
     * @param[in] present - What the present property should be set to.
     */
    void updateInventory(bool present);

    /**
     * @brief Construct the inventory object map for the inventory item.
     *
     * @param[in] present - What the present property should be set to.
     *
     * @return The inventory object map to update inventory
     */
    ObjectMap getObjectMap(bool present);

    /** @brief Connection for sdbusplus bus */
    sdbusplus::bus::bus& bus;

    /**
     * @brief Read the GPIO device to determine initial presence and set
     *        present property at D-Bus path.
     */
    void determinePresence();

    /** @brief Object path under inventory to display this inventory item */
    const std::string inventory;

    /** @brief Pretty name of the inventory item*/
    const std::string name;

    /** @brief Analyzes the GPIO event and update present property*/
    void analyzeEvent();

    /** @brief  Vector of path and device tuples to bind/unbind*/
    const std::vector<Driver> drivers;

    /** @brief  Vector of extra inventory interfaces to associate with the
     *          inventory item
     */
    const std::vector<Interface> ifaces;

    /**
     * @brief Binds or unbinds drivers
     *
     * Called when a presence change is detected to either
     * bind the drivers for the new card or unbind them for
     * the just removed card.  Operates on the drivers vector.
     *
     * Writes <device> to <path>/bind (or unbind)
     *
     * @param present - when true, will bind the drivers
     *                  when false, will unbind them
     */
    void bindOrUnbindDrivers(bool present);
};

/**
 * @brief Get the service name from the mapper for the
 *        interface and path passed in.
 *
 * @param[in] path      - The D-Bus path name
 * @param[in] interface - The D-Bus interface name
 * @param[in] bus       - The D-Bus bus object
 *
 * @return The service name
 */
std::string getService(const std::string& path, const std::string& interface,
                       sdbusplus::bus::bus& bus);

} // namespace presence
} // namespace gpio
} // namespace phosphor
