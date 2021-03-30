#include "config.h"

#include "chassis_state_manager.hpp"

#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/State/Shutdown/Power/error.hpp"

#include <cereal/archives/json.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/exception.hpp>

#include <experimental/filesystem>
#include <fstream>

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;

using namespace phosphor::logging;
using sdbusplus::exception::SdBusError;
using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using sdbusplus::xyz::openbmc_project::State::Shutdown::Power::Error::Blackout;
constexpr auto CHASSIS_STATE_POWEROFF_TGT = "obmc-chassis-poweroff@0.target";
constexpr auto CHASSIS_STATE_HARD_POWEROFF_TGT =
    "obmc-chassis-hard-poweroff@0.target";
constexpr auto CHASSIS_STATE_POWERON_TGT = "obmc-chassis-poweron@0.target";

constexpr auto ACTIVE_STATE = "active";
constexpr auto ACTIVATING_STATE = "activating";

/* Map a transition to it's systemd target */
const std::map<server::Chassis::Transition, std::string> SYSTEMD_TARGET_TABLE =
    {
        // Use the hard off target to ensure we shutdown immediately
        {server::Chassis::Transition::Off, CHASSIS_STATE_HARD_POWEROFF_TGT},
        {server::Chassis::Transition::On, CHASSIS_STATE_POWERON_TGT}};

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto SYSTEMD_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";
constexpr auto SYSTEMD_INTERFACE_UNIT = "org.freedesktop.systemd1.Unit";

void Chassis::subscribeToSystemdSignals()
{
    try
    {
        auto method = this->bus.new_method_call(
            SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE, "Subscribe");
        this->bus.call(method);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("Failed to subscribe to systemd signals",
                        entry("ERR=%s", ex.what()));
        elog<InternalFailure>();
    }

    return;
}

// TODO - Will be rewritten once sdbusplus client bindings are in place
//        and persistent storage design is in place and sdbusplus
//        has read property function
void Chassis::determineInitialState()
{
    std::variant<int> pgood = -1;
    auto method = this->bus.new_method_call(
        "org.openbmc.control.Power", "/org/openbmc/control/power0",
        "org.freedesktop.DBus.Properties", "Get");

    method.append("org.openbmc.control.Power", "pgood");
    try
    {
        auto reply = this->bus.call(method);
        reply.read(pgood);

        if (std::get<int>(pgood) == 1)
        {
            log<level::INFO>("Initial Chassis State will be On",
                             entry("CHASSIS_CURRENT_POWER_STATE=%s",
                                   convertForMessage(PowerState::On).c_str()));
            server::Chassis::currentPowerState(PowerState::On);
            server::Chassis::requestedPowerTransition(Transition::On);
            return;
        }
        else
        {
            // The system is off.  If we think it should be on then
            // we probably lost AC while up, so set a new state
            // change time.
            uint64_t lastTime;
            PowerState lastState;

            if (deserializeStateChangeTime(lastTime, lastState))
            {
                if (lastState == PowerState::On)
                {
                    report<Blackout>();
                    setStateChangeTime();
                }
            }
        }
    }
    catch (const SdBusError& e)
    {
        // It's acceptable for the pgood state service to not be available
        // since it will notify us of the pgood state when it comes up.
        if (e.name() != nullptr &&
            strcmp("org.freedesktop.DBus.Error.ServiceUnknown", e.name()) == 0)
        {
            goto fail;
        }

        // Only log for unexpected error types.
        log<level::ERR>("Error performing call to get pgood",
                        entry("ERROR=%s", e.what()));
        goto fail;
    }

fail:
    log<level::INFO>("Initial Chassis State will be Off",
                     entry("CHASSIS_CURRENT_POWER_STATE=%s",
                           convertForMessage(PowerState::Off).c_str()));
    server::Chassis::currentPowerState(PowerState::Off);
    server::Chassis::requestedPowerTransition(Transition::Off);

    return;
}

void Chassis::executeTransition(Transition tranReq)
{
    auto sysdTarget = SYSTEMD_TARGET_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "StartUnit");

    method.append(sysdTarget);
    method.append("replace");

    this->bus.call_noreply(method);

    return;
}

bool Chassis::stateActive(const std::string& target)
{
    std::variant<std::string> currentState;
    sdbusplus::message::object_path unitTargetPath;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "GetUnit");

    method.append(target);

    try
    {
        auto result = this->bus.call(method);
        result.read(unitTargetPath);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in GetUnit call", entry("ERROR=%s", e.what()));
        return false;
    }

    method = this->bus.new_method_call(
        SYSTEMD_SERVICE,
        static_cast<const std::string&>(unitTargetPath).c_str(),
        SYSTEMD_PROPERTY_IFACE, "Get");

    method.append(SYSTEMD_INTERFACE_UNIT, "ActiveState");

    try
    {
        auto result = this->bus.call(method);
        result.read(currentState);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in ActiveState Get",
                        entry("ERROR=%s", e.what()));
        return false;
    }

    const auto& currentStateStr = std::get<std::string>(currentState);
    return currentStateStr == ACTIVE_STATE ||
           currentStateStr == ACTIVATING_STATE;
}

int Chassis::sysStateChange(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    try
    {
        // newStateID is a throwaway that is needed in order to read the
        // parameters that are useful out of the dbus message
        uint32_t newStateID{};
        msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in state change - bad encoding",
                        entry("ERROR=%s", e.what()),
                        entry("REPLY_SIG=%s", msg.get_signature()));
        return 0;
    }

    if ((newStateUnit == CHASSIS_STATE_POWEROFF_TGT) &&
        (newStateResult == "done") && (!stateActive(CHASSIS_STATE_POWERON_TGT)))
    {
        log<level::INFO>("Received signal that power OFF is complete");
        this->currentPowerState(server::Chassis::PowerState::Off);
        this->setStateChangeTime();
    }
    else if ((newStateUnit == CHASSIS_STATE_POWERON_TGT) &&
             (newStateResult == "done") &&
             (stateActive(CHASSIS_STATE_POWERON_TGT)))
    {
        log<level::INFO>("Received signal that power ON is complete");
        this->currentPowerState(server::Chassis::PowerState::On);
        this->setStateChangeTime();
    }

    return 0;
}

Chassis::Transition Chassis::requestedPowerTransition(Transition value)
{

    log<level::INFO>("Change to Chassis Requested Power State",
                     entry("CHASSIS_REQUESTED_POWER_STATE=%s",
                           convertForMessage(value).c_str()));
    executeTransition(value);
    return server::Chassis::requestedPowerTransition(value);
}

Chassis::PowerState Chassis::currentPowerState(PowerState value)
{
    PowerState chassisPowerState;
    log<level::INFO>("Change to Chassis Power State",
                     entry("CHASSIS_CURRENT_POWER_STATE=%s",
                           convertForMessage(value).c_str()));

    chassisPowerState = server::Chassis::currentPowerState(value);
    pOHTimer.setEnabled(chassisPowerState == PowerState::On);
    return chassisPowerState;
}

uint32_t Chassis::pOHCounter(uint32_t value)
{
    if (value != pOHCounter())
    {
        ChassisInherit::pOHCounter(value);
        serializePOH();
    }
    return pOHCounter();
}

void Chassis::pOHCallback()
{
    if (ChassisInherit::currentPowerState() == PowerState::On)
    {
        pOHCounter(pOHCounter() + 1);
    }
}

void Chassis::restorePOHCounter()
{
    uint32_t counter;
    if (!deserializePOH(POH_COUNTER_PERSIST_PATH, counter))
    {
        // set to default value
        pOHCounter(0);
    }
    else
    {
        pOHCounter(counter);
    }
}

fs::path Chassis::serializePOH(const fs::path& path)
{
    std::ofstream os(path.c_str(), std::ios::binary);
    cereal::JSONOutputArchive oarchive(os);
    oarchive(pOHCounter());
    return path;
}

bool Chassis::deserializePOH(const fs::path& path, uint32_t& pOHCounter)
{
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(pOHCounter);
            return true;
        }
        return false;
    }
    catch (cereal::Exception& e)
    {
        log<level::ERR>(e.what());
        fs::remove(path);
        return false;
    }
    catch (const fs::filesystem_error& e)
    {
        return false;
    }

    return false;
}

void Chassis::startPOHCounter()
{
    auto dir = fs::path(POH_COUNTER_PERSIST_PATH).parent_path();
    fs::create_directories(dir);

    try
    {
        auto event = sdeventplus::Event::get_default();
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
        event.loop();
    }
    catch (const sdeventplus::SdEventError& e)
    {
        log<level::ERR>("Error occurred during the sdeventplus loop",
                        entry("ERROR=%s", e.what()));
        phosphor::logging::commit<InternalFailure>();
    }
}

void Chassis::serializeStateChangeTime()
{
    fs::path path{CHASSIS_STATE_CHANGE_PERSIST_PATH};
    std::ofstream os(path.c_str(), std::ios::binary);
    cereal::JSONOutputArchive oarchive(os);

    oarchive(ChassisInherit::lastStateChangeTime(),
             ChassisInherit::currentPowerState());
}

bool Chassis::deserializeStateChangeTime(uint64_t& time, PowerState& state)
{
    fs::path path{CHASSIS_STATE_CHANGE_PERSIST_PATH};

    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(time, state);
            return true;
        }
    }
    catch (std::exception& e)
    {
        log<level::ERR>(e.what());
        fs::remove(path);
    }

    return false;
}

void Chassis::restoreChassisStateChangeTime()
{
    uint64_t time;
    PowerState state;

    if (!deserializeStateChangeTime(time, state))
    {
        ChassisInherit::lastStateChangeTime(0);
    }
    else
    {
        ChassisInherit::lastStateChangeTime(time);
    }
}

void Chassis::setStateChangeTime()
{
    using namespace std::chrono;
    uint64_t lastTime;
    PowerState lastState;

    auto now =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count();

    // If power is on when the BMC is rebooted, this function will get called
    // because sysStateChange() runs.  Since the power state didn't change
    // in this case, neither should the state change time, so check that
    // the power state actually did change here.
    if (deserializeStateChangeTime(lastTime, lastState))
    {
        if (lastState == ChassisInherit::currentPowerState())
        {
            return;
        }
    }

    ChassisInherit::lastStateChangeTime(now);
    serializeStateChangeTime();
}

} // namespace manager
} // namespace state
} // namespace phosphor
