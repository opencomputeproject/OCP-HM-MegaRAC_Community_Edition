#include "bmc_state_manager.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <sys/sysinfo.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>

#include <cassert>

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

constexpr auto obmcStandbyTarget = "multi-user.target";
constexpr auto signalDone = "done";
constexpr auto activeState = "active";

/* Map a transition to it's systemd target */
const std::map<server::BMC::Transition, const char*> SYSTEMD_TABLE = {
    {server::BMC::Transition::Reboot, "reboot.target"}};

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto SYSTEMD_PRP_INTERFACE = "org.freedesktop.DBus.Properties";

void BMC::discoverInitialState()
{
    std::variant<std::string> currentState;
    sdbusplus::message::object_path unitTargetPath;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "GetUnit");

    method.append(obmcStandbyTarget);

    try
    {
        auto result = this->bus.call(method);
        result.read(unitTargetPath);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in GetUnit call", entry("ERROR=%s", e.what()));
        return;
    }

    method = this->bus.new_method_call(
        SYSTEMD_SERVICE,
        static_cast<const std::string&>(unitTargetPath).c_str(),
        SYSTEMD_PRP_INTERFACE, "Get");

    method.append("org.freedesktop.systemd1.Unit", "ActiveState");

    try
    {
        auto result = this->bus.call(method);

        // Is obmc-standby.target active or inactive?
        result.read(currentState);
    }
    catch (const SdBusError& e)
    {
        log<level::INFO>("Error in ActiveState Get",
                         entry("ERROR=%s", e.what()));
        return;
    }

    auto currentStateStr = std::get<std::string>(currentState);
    if (currentStateStr == activeState)
    {
        log<level::INFO>("Setting the BMCState field",
                         entry("CURRENT_BMC_STATE=%s", "BMC_READY"));
        this->currentBMCState(BMCState::Ready);

        // Unsubscribe so we stop processing all other signals
        method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                           SYSTEMD_INTERFACE, "Unsubscribe");
        try
        {
            this->bus.call(method);
            this->stateSignal.release();
        }
        catch (const SdBusError& e)
        {
            log<level::INFO>("Error in Unsubscribe",
                             entry("ERROR=%s", e.what()));
        }
    }
    else
    {
        log<level::INFO>("Setting the BMCState field",
                         entry("CURRENT_BMC_STATE=%s", "BMC_NOTREADY"));
        this->currentBMCState(BMCState::NotReady);
    }

    return;
}

void BMC::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "Subscribe");

    try
    {
        this->bus.call(method);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Failed to subscribe to systemd signals",
                        entry("ERR=%s", e.what()));
        elog<InternalFailure>();
    }

    return;
}

void BMC::executeTransition(const Transition tranReq)
{
    // Check to make sure it can be found
    auto iter = SYSTEMD_TABLE.find(tranReq);
    if (iter == SYSTEMD_TABLE.end())
        return;

    const auto& sysdUnit = iter->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "StartUnit");
    // The only valid transition is reboot and that
    // needs to be irreversible once started
    method.append(sysdUnit, "replace-irreversibly");

    try
    {
        this->bus.call(method);
    }
    catch (const SdBusError& e)
    {
        log<level::INFO>("Error in StartUnit - replace-irreversibly",
                         entry("ERROR=%s", e.what()));
    }

    return;
}

int BMC::bmcStateChange(sdbusplus::message::message& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    // Caught the signal that indicates the BMC is now BMC_READY
    if ((newStateUnit == obmcStandbyTarget) && (newStateResult == signalDone))
    {
        log<level::INFO>("BMC_READY");
        this->currentBMCState(BMCState::Ready);

        // Unsubscribe so we stop processing all other signals
        auto method =
            this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_INTERFACE, "Unsubscribe");

        try
        {
            this->bus.call(method);
            this->stateSignal.release();
        }
        catch (const SdBusError& e)
        {
            log<level::INFO>("Error in Unsubscribe",
                             entry("ERROR=%s", e.what()));
        }
    }

    return 0;
}

BMC::Transition BMC::requestedBMCTransition(Transition value)
{
    log<level::INFO>("Setting the RequestedBMCTransition field",
                     entry("REQUESTED_BMC_TRANSITION=0x%s",
                           convertForMessage(value).c_str()));

    executeTransition(value);
    return server::BMC::requestedBMCTransition(value);
}

BMC::BMCState BMC::currentBMCState(BMCState value)
{
    log<level::INFO>(
        "Setting the BMCState field",
        entry("CURRENT_BMC_STATE=0x%s", convertForMessage(value).c_str()));

    return server::BMC::currentBMCState(value);
}

uint64_t BMC::lastRebootTime() const
{
    using namespace std::chrono;
    struct sysinfo info;

    auto rc = sysinfo(&info);
    assert(rc == 0);

    // Since uptime is in seconds, also get the current time in seconds.
    auto now = time_point_cast<seconds>(system_clock::now());
    auto rebootTime = now - seconds(info.uptime);

    return duration_cast<milliseconds>(rebootTime.time_since_epoch()).count();
}

} // namespace manager
} // namespace state
} // namespace phosphor
