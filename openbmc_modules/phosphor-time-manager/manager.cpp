#include "manager.hpp"

#include "utils.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

namespace rules = sdbusplus::bus::match::rules;

namespace // anonymous
{
constexpr auto HOST_CURRENT_STATE = "CurrentHostState";

constexpr auto SYSTEMD_TIME_SERVICE = "org.freedesktop.timedate1";
constexpr auto SYSTEMD_TIME_PATH = "/org/freedesktop/timedate1";
constexpr auto SYSTEMD_TIME_INTERFACE = "org.freedesktop.timedate1";
constexpr auto METHOD_SET_NTP = "SetNTP";
} // namespace

namespace phosphor
{
namespace time
{

using namespace phosphor::logging;

const std::set<std::string> Manager::managedProperties = {PROPERTY_TIME_MODE};

Manager::Manager(sdbusplus::bus::bus& bus) : bus(bus), settings(bus)
{
    using namespace sdbusplus::bus::match::rules;
    hostStateChangeMatch =
        std::make_unique<decltype(hostStateChangeMatch)::element_type>(
            bus, propertiesChanged(settings.hostState, settings::hostStateIntf),
            std::bind(std::mem_fn(&Manager::onHostStateChanged), this,
                      std::placeholders::_1));
    settingsMatches.emplace_back(
        bus, propertiesChanged(settings.timeSyncMethod, settings::timeSyncIntf),
        std::bind(std::mem_fn(&Manager::onSettingsChanged), this,
                  std::placeholders::_1));

    checkHostOn();

    // Restore settings from persistent storage
    restoreSettings();

    // Check the settings daemon to process the new settings
    auto mode = getSetting(settings.timeSyncMethod.c_str(),
                           settings::timeSyncIntf, PROPERTY_TIME_MODE);

    onPropertyChanged(PROPERTY_TIME_MODE, mode);
}

void Manager::addListener(PropertyChangeListner* listener)
{
    // Notify listener about the initial value
    listener->onModeChanged(timeMode);

    listeners.insert(listener);
}

void Manager::restoreSettings()
{
    auto mode = utils::readData<std::string>(modeFile);
    if (!mode.empty())
    {
        timeMode = utils::strToMode(mode);
    }
}

void Manager::checkHostOn()
{
    using Host = sdbusplus::xyz::openbmc_project::State::server::Host;
    auto hostService = utils::getService(bus, settings.hostState.c_str(),
                                         settings::hostStateIntf);
    auto stateStr = utils::getProperty<std::string>(
        bus, hostService.c_str(), settings.hostState.c_str(),
        settings::hostStateIntf, HOST_CURRENT_STATE);
    auto state = Host::convertHostStateFromString(stateStr);
    hostOn = (state == Host::HostState::Running);
}

void Manager::onPropertyChanged(const std::string& key,
                                const std::string& value)
{
    if (hostOn)
    {
        // If host is on, set the values as requested time mode.
        // And when host becomes off, notify the listeners.
        setPropertyAsRequested(key, value);
    }
    else
    {
        // If host is off, notify listeners
        if (key == PROPERTY_TIME_MODE)
        {
            setCurrentTimeMode(value);
            onTimeModeChanged(value);
        }
    }
}

int Manager::onSettingsChanged(sdbusplus::message::message& msg)
{
    using Interface = std::string;
    using Property = std::string;
    using Value = std::string;
    using Properties = std::map<Property, std::variant<Value>>;

    Interface interface;
    Properties properties;

    msg.read(interface, properties);

    for (const auto& p : properties)
    {
        onPropertyChanged(p.first, std::get<std::string>(p.second));
    }

    return 0;
}

void Manager::setPropertyAsRequested(const std::string& key,
                                     const std::string& value)
{
    if (key == PROPERTY_TIME_MODE)
    {
        setRequestedMode(value);
    }
    else
    {
        // The key shall be already the supported one
        using InvalidArgumentError =
            sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
        using namespace xyz::openbmc_project::Common;
        elog<InvalidArgumentError>(
            InvalidArgument::ARGUMENT_NAME(key.c_str()),
            InvalidArgument::ARGUMENT_VALUE(value.c_str()));
    }
}

void Manager::setRequestedMode(const std::string& mode)
{
    requestedMode = mode;
}

void Manager::updateNtpSetting(const std::string& value)
{
    bool isNtp =
        (value == "xyz.openbmc_project.Time.Synchronization.Method.NTP");
    auto method = bus.new_method_call(SYSTEMD_TIME_SERVICE, SYSTEMD_TIME_PATH,
                                      SYSTEMD_TIME_INTERFACE, METHOD_SET_NTP);
    method.append(isNtp, false); // isNtp: 'true/false' means Enable/Disable
                                 // 'false' meaning no policy-kit

    try
    {
        bus.call_noreply(method);
        log<level::INFO>("Updated NTP setting", entry("ENABLED=%d", isNtp));
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("Failed to update NTP setting",
                        entry("ERR=%s", ex.what()));
    }
}

void Manager::onHostStateChanged(sdbusplus::message::message& msg)
{
    using Interface = std::string;
    using Property = std::string;
    using Value = std::string;
    using Properties = std::map<Property, std::variant<Value>>;
    using Host = sdbusplus::xyz::openbmc_project::State::server::Host;

    Interface interface;
    Properties properties;

    msg.read(interface, properties);

    for (const auto& p : properties)
    {
        if (p.first == HOST_CURRENT_STATE)
        {
            auto state = Host::convertHostStateFromString(
                std::get<std::string>(p.second));
            onHostState(state == Host::HostState::Running);
            break;
        }
    }
}

void Manager::onHostState(bool on)
{
    hostOn = on;
    if (hostOn)
    {
        log<level::INFO>("Changing time settings is *deferred* now");
        return;
    }
    log<level::INFO>("Changing time settings allowed now");
    if (!requestedMode.empty())
    {
        if (setCurrentTimeMode(requestedMode))
        {
            onTimeModeChanged(requestedMode);
        }
        setRequestedMode({}); // Clear requested mode
    }
}

bool Manager::setCurrentTimeMode(const std::string& mode)
{
    auto newMode = utils::strToMode(mode);
    if (newMode != timeMode)
    {
        log<level::INFO>("Time mode is changed",
                         entry("MODE=%s", mode.c_str()));
        timeMode = newMode;
        utils::writeData(modeFile, mode);
        return true;
    }
    else
    {
        return false;
    }
}

void Manager::onTimeModeChanged(const std::string& mode)
{
    for (const auto listener : listeners)
    {
        listener->onModeChanged(timeMode);
    }
    // When time_mode is updated, update the NTP setting
    updateNtpSetting(mode);
}

std::string Manager::getSetting(const char* path, const char* interface,
                                const char* setting) const
{
    std::string settingManager = utils::getService(bus, path, interface);
    return utils::getProperty<std::string>(bus, settingManager.c_str(), path,
                                           interface, setting);
}

} // namespace time
} // namespace phosphor
