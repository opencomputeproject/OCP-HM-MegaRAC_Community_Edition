#pragma once
#include "file.hpp"

#include <libevdev/libevdev.h>
#include <systemd/sd-event.h>

#include <map>
#include <memory>
#include <sdbusplus/message.hpp>
#include <string>

namespace phosphor
{
namespace gpio
{

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};
using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

/* Need a custom deleter for freeing up sd_event_source */
struct EventSourceDeleter
{
    void operator()(sd_event_source* eventSource) const
    {
        eventSource = sd_event_source_unref(eventSource);
    }
};
using EventSourcePtr = std::unique_ptr<sd_event_source, EventSourceDeleter>;

/* Need a custom deleter for freeing up evdev struct */
struct FreeEvDev
{
    void operator()(struct libevdev* device) const
    {
        libevdev_free(device);
    }
};
using EvdevPtr = std::unique_ptr<struct libevdev, FreeEvDev>;

/** @class Evdev
 *  @brief Responsible for catching GPIO state changes conditions and taking
 *  actions
 */
class Evdev
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
    Evdev() = delete;
    ~Evdev() = default;
    Evdev(const Evdev&) = delete;
    Evdev& operator=(const Evdev&) = delete;
    Evdev(Evdev&&) = delete;
    Evdev& operator=(Evdev&&) = delete;

    /** @brief Constructs Evdev object.
     *
     *  @param[in] path      - Device path to read for GPIO pin state
     *  @param[in] key       - GPIO key to monitor
     *  @param[in] event     - sd_event handler
     *  @param[in] handler   - IO callback handler.
     *  @param[in] useEvDev  - Whether to use EvDev to retrieve events
     */
    Evdev(const std::string& path, const unsigned int key, EventPtr& event,
          sd_event_io_handler_t handler, bool useEvDev = true) :
        path(path),
        key(key), event(event), callbackHandler(handler), fd(openDevice())

    {
        if (useEvDev)
        {
            // If we are asked to use EvDev, do that initialization.
            initEvDev();
        }

        // Register callback handler when FD has some data
        registerCallback();
    }

  protected:
    /** @brief Device path to read for GPIO pin state */
    const std::string path;

    /** @brief GPIO key to monitor */
    const unsigned int key;

    /** @brief Event structure */
    EvdevPtr devicePtr;

    /** @brief Monitor to sd_event */
    EventPtr& event;

    /** @brief Callback handler when the FD has some data */
    sd_event_io_handler_t callbackHandler;

    /** @brief event source */
    EventSourcePtr eventSource;

    /** @brief Opens the device and populates the descriptor */
    int openDevice();

    /** @brief attaches FD to events and sets up callback handler */
    void registerCallback();

    /** @brief File descriptor manager */
    FileDescriptor fd;

    /** @brief Initializes evdev handle with the fd */
    void initEvDev();
};

} // namespace gpio
} // namespace phosphor
