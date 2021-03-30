#pragma once

#include "config.h"

#include "property_change_listener.hpp"
#include "settings.hpp"
#include "types.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <set>
#include <string>

namespace phosphor
{
namespace time
{

/** @class Manager
 *  @brief The manager to handle OpenBMC time.
 *  @details It registers various time related settings and properties signals
 *  on DBus and handle the changes.
 *  For certain properties it also notifies the changed events to listeners.
 */
class Manager
{
  public:
    friend class TestManager;

    explicit Manager(sdbusplus::bus::bus& bus);
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /** @brief Add a listener that will be called
     * when property is changed
     **/
    void addListener(PropertyChangeListner* listener);

  private:
    /** @brief Persistent sdbusplus DBus connection */
    sdbusplus::bus::bus& bus;

    /** @brief The match of settings property change */
    std::vector<sdbusplus::bus::match::match> settingsMatches;

    /** @brief The match of host state change */
    std::unique_ptr<sdbusplus::bus::match::match> hostStateChangeMatch;

    /** @brief The container to hold all the listeners */
    std::set<PropertyChangeListner*> listeners;

    /** @brief Settings objects of intereset */
    settings::Objects settings;

    /** @brief The value to indicate if host is on */
    bool hostOn = false;

    /** @brief The requested time mode when host is on*/
    std::string requestedMode;

    /** @brief The current time mode */
    Mode timeMode = DEFAULT_TIME_MODE;

    /** @brief Restore saved settings */
    void restoreSettings();

    /** @brief Check if host is on and update hostOn variable */
    void checkHostOn();

    /** @brief Get setting from settingsd service
     *
     * @param[in] path - The dbus object path
     * @param[in] interface - The dbus interface
     * @param[in] setting - The string of the setting
     *
     * @return The setting value in string
     */
    std::string getSetting(const char* path, const char* interface,
                           const char* setting) const;

    /** @brief Set current time mode from the time mode string
     *
     * @param[in] mode - The string of time mode
     *
     * @return - true if the mode is updated
     *           false if it's the same as before
     */
    bool setCurrentTimeMode(const std::string& mode);

    /** @brief Called on time mode is changed
     *
     * Notify listeners that time mode is changed and update ntp setting
     *
     * @param[in] mode - The string of time mode
     */
    void onTimeModeChanged(const std::string& mode);

    /** @brief Callback to handle change in a setting
     *
     *  @param[in] msg - sdbusplus dbusmessage
     *
     *  @return 0 on success, < 0 on failure.
     */
    int onSettingsChanged(sdbusplus::message::message& msg);

    /** @brief Notified on settings property changed
     *
     * @param[in] key - The name of property that is changed
     * @param[in] value - The value of the property
     */
    void onPropertyChanged(const std::string& key, const std::string& value);

    /** @brief Notified on host state has changed
     *
     * @param[in] msg - sdbusplus dbusmessage
     */
    void onHostStateChanged(sdbusplus::message::message& msg);

    /** @brief Notified on host state has changed
     *
     * @param[in] on - Indicate if the host is on or off
     */
    void onHostState(bool on);

    /** @brief Set the property as requested time mode
     *
     * @param[in] key - The property name
     * @param[in] value - The property value
     */
    void setPropertyAsRequested(const std::string& key,
                                const std::string& value);

    /** @brief Set the current mode to user requested one
     *  if conditions allow it
     *
     * @param[in] mode - The string of time mode
     */
    void setRequestedMode(const std::string& mode);

    /** @brief Update the NTP setting to systemd time service
     *
     * @param[in] value - The time mode value, e.g. "NTP" or "MANUAL"
     */
    void updateNtpSetting(const std::string& value);

    /** @brief The static function called on settings property changed
     *
     * @param[in] msg - Data associated with subscribed signal
     * @param[in] userData - Pointer to this object instance
     * @param[out] retError  - Not used but required with signal API
     */
    static int onPropertyChanged(sd_bus_message* msg, void* userData,
                                 sd_bus_error* retError);

    /** @brief The string of time mode property */
    static constexpr auto PROPERTY_TIME_MODE = "TimeSyncMethod";

    using Updater = std::function<void(const std::string&)>;

    /** @brief Map the property string to functions that shall
     *  be called when the property is changed
     */
    const std::map<std::string, Updater> propertyUpdaters = {
        {PROPERTY_TIME_MODE,
         std::bind(&Manager::setCurrentTimeMode, this, std::placeholders::_1)}};

    /** @brief The properties that manager shall notify the
     *  listeners when changed
     */
    static const std::set<std::string> managedProperties;

    /** @brief The file name of saved time mode */
    static constexpr auto modeFile = "/var/lib/obmc/saved_time_mode";
};

} // namespace time
} // namespace phosphor
