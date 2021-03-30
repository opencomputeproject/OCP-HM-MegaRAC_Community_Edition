#include "config.h"

#include "host_state_manager.hpp"

#include <systemd/sd-bus.h>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Control/Power/RestorePolicy/server.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <string>

// Register class version with Cereal
CEREAL_CLASS_VERSION(phosphor::state::manager::Host, CLASS_VERSION)

namespace phosphor
{
namespace state
{
namespace manager
{

// When you see server:: or reboot:: you know we're referencing our base class
namespace server = sdbusplus::xyz::openbmc_project::State::server;
namespace reboot = sdbusplus::xyz::openbmc_project::Control::Boot::server;
namespace bootprogress = sdbusplus::xyz::openbmc_project::State::Boot::server;
namespace osstatus =
    sdbusplus::xyz::openbmc_project::State::OperatingSystem::server;
using namespace phosphor::logging;
namespace fs = std::experimental::filesystem;
using sdbusplus::exception::SdBusError;
using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

// host-shutdown notifies host of shutdown and that leads to host-stop being
// called so initiate a host shutdown with the -shutdown target and consider the
// host shut down when the -stop target is complete
constexpr auto HOST_STATE_SOFT_POWEROFF_TGT = "obmc-host-shutdown@0.target";
constexpr auto HOST_STATE_POWEROFF_TGT = "obmc-host-stop@0.target";
constexpr auto HOST_STATE_POWERON_TGT = "obmc-host-start@0.target";
constexpr auto HOST_STATE_POWERON_MIN_TGT = "obmc-host-startmin@0.target";
constexpr auto HOST_STATE_REBOOT_TGT = "obmc-host-reboot@0.target";
constexpr auto HOST_STATE_WARM_REBOOT = "obmc-host-warm-reboot@0.target";
constexpr auto HOST_STATE_FORCE_WARM_REBOOT =
    "obmc-host-force-warm-reboot@0.target";
constexpr auto HOST_STATE_DIAGNOSTIC_MODE =
    "obmc-host-diagnostic-mode@0.target";

constexpr auto HOST_STATE_QUIESCE_TGT = "obmc-host-quiesce@0.target";

constexpr auto ACTIVE_STATE = "active";
constexpr auto ACTIVATING_STATE = "activating";

/* Map a transition to it's systemd target */
const std::map<server::Host::Transition, std::string> SYSTEMD_TARGET_TABLE = {
    {server::Host::Transition::Off, HOST_STATE_SOFT_POWEROFF_TGT},
    {server::Host::Transition::On, HOST_STATE_POWERON_TGT},
    {server::Host::Transition::Reboot, HOST_STATE_REBOOT_TGT},
// Some systems do not support a warm reboot so just map the reboot
// requests to our normal cold reboot in that case
#if ENABLE_WARM_REBOOT
    {server::Host::Transition::GracefulWarmReboot, HOST_STATE_WARM_REBOOT},
    {server::Host::Transition::ForceWarmReboot, HOST_STATE_FORCE_WARM_REBOOT}};
#else
    {server::Host::Transition::GracefulWarmReboot, HOST_STATE_REBOOT_TGT},
    {server::Host::Transition::ForceWarmReboot, HOST_STATE_REBOOT_TGT}};
#endif

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto SYSTEMD_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";
constexpr auto SYSTEMD_INTERFACE_UNIT = "org.freedesktop.systemd1.Unit";

void Host::subscribeToSystemdSignals()
{
    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "Subscribe");
    try
    {
        this->bus.call_noreply(method);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Failed to subscribe to systemd signals",
                        entry("ERR=%s", e.what()));
        elog<InternalFailure>();
    }
    return;
}

void Host::determineInitialState()
{

    if (stateActive(HOST_STATE_POWERON_MIN_TGT))
    {
        log<level::INFO>("Initial Host State will be Running",
                         entry("CURRENT_HOST_STATE=%s",
                               convertForMessage(HostState::Running).c_str()));
        server::Host::currentHostState(HostState::Running);
        server::Host::requestedHostTransition(Transition::On);
    }
    else
    {
        log<level::INFO>("Initial Host State will be Off",
                         entry("CURRENT_HOST_STATE=%s",
                               convertForMessage(HostState::Off).c_str()));
        server::Host::currentHostState(HostState::Off);
        server::Host::requestedHostTransition(Transition::Off);
    }

    if (!deserialize(HOST_STATE_PERSIST_PATH))
    {
        // set to default value.
        server::Host::requestedHostTransition(Transition::Off);
    }

    return;
}

void Host::executeTransition(Transition tranReq)
{
    auto sysdUnit = SYSTEMD_TARGET_TABLE.find(tranReq)->second;

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, "StartUnit");

    method.append(sysdUnit);
    method.append("replace");

    this->bus.call_noreply(method);

    return;
}

bool Host::stateActive(const std::string& target)
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

bool Host::isAutoReboot()
{
    using namespace settings;

    auto method = bus.new_method_call(
        settings.service(settings.autoReboot, autoRebootIntf).c_str(),
        settings.autoReboot.c_str(), "org.freedesktop.DBus.Properties", "Get");
    method.append(autoRebootIntf, "AutoReboot");

    try
    {
        auto reply = bus.call(method);

        std::variant<bool> result;
        reply.read(result);
        auto autoReboot = std::get<bool>(result);
        auto rebootCounterParam = reboot::RebootAttempts::attemptsLeft();

        if (autoReboot)
        {
            if (rebootCounterParam > 0)
            {
                // Reduce BOOTCOUNT by 1
                log<level::INFO>("Auto reboot enabled, rebooting");
                return true;
            }
            else if (rebootCounterParam == 0)
            {
                // Reset reboot counter and go to quiesce state
                log<level::INFO>("Auto reboot enabled. "
                                 "HOST BOOTCOUNT already set to 0.");
                attemptsLeft(BOOT_COUNT_MAX_ALLOWED);
                return false;
            }
            else
            {
                log<level::INFO>("Auto reboot enabled. "
                                 "HOST BOOTCOUNT has an invalid value.");
                return false;
            }
        }
        else
        {
            log<level::INFO>("Auto reboot disabled.");
            return false;
        }
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in AutoReboot Get", entry("ERROR=%s", e.what()));
        return false;
    }
}

void Host::sysStateChangeJobRemoved(sdbusplus::message::message& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};
    std::string newStateResult{};

    // Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit, newStateResult);

    if ((newStateUnit == HOST_STATE_POWEROFF_TGT) &&
        (newStateResult == "done") &&
        (!stateActive(HOST_STATE_POWERON_MIN_TGT)))
    {
        log<level::INFO>("Received signal that host is off");
        this->currentHostState(server::Host::HostState::Off);
        this->bootProgress(bootprogress::Progress::ProgressStages::Unspecified);
        this->operatingSystemState(osstatus::Status::OSStatus::Inactive);
    }
    else if ((newStateUnit == HOST_STATE_POWERON_MIN_TGT) &&
             (newStateResult == "done") &&
             (stateActive(HOST_STATE_POWERON_MIN_TGT)))
    {
        log<level::INFO>("Received signal that host is running");
        this->currentHostState(server::Host::HostState::Running);
    }
    else if ((newStateUnit == HOST_STATE_QUIESCE_TGT) &&
             (newStateResult == "done") &&
             (stateActive(HOST_STATE_QUIESCE_TGT)))
    {
        if (Host::isAutoReboot())
        {
            log<level::INFO>("Beginning reboot...");
            Host::requestedHostTransition(server::Host::Transition::Reboot);
        }
        else
        {
            log<level::INFO>("Maintaining quiesce");
            this->currentHostState(server::Host::HostState::Quiesced);
        }
    }
}

void Host::sysStateChangeJobNew(sdbusplus::message::message& msg)
{
    uint32_t newStateID{};
    sdbusplus::message::object_path newStateObjPath;
    std::string newStateUnit{};

    // Read the msg and populate each variable
    msg.read(newStateID, newStateObjPath, newStateUnit);

    if (newStateUnit == HOST_STATE_DIAGNOSTIC_MODE)
    {
        log<level::INFO>("Received signal that host is in diagnostice mode");
        this->currentHostState(server::Host::HostState::DiagnosticMode);
    }
}

uint32_t Host::decrementRebootCount()
{
    auto rebootCount = reboot::RebootAttempts::attemptsLeft();
    if (rebootCount > 0)
    {
        return (reboot::RebootAttempts::attemptsLeft(rebootCount - 1));
    }
    return rebootCount;
}

fs::path Host::serialize(const fs::path& dir)
{
    std::ofstream os(dir.c_str(), std::ios::binary);
    cereal::JSONOutputArchive oarchive(os);
    oarchive(*this);
    return dir;
}

bool Host::deserialize(const fs::path& path)
{
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(*this);
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
}

Host::Transition Host::requestedHostTransition(Transition value)
{
    log<level::INFO>("Host State transaction request",
                     entry("REQUESTED_HOST_TRANSITION=%s",
                           convertForMessage(value).c_str()));

    // If this is not a power off request then we need to
    // decrement the reboot counter.  This code should
    // never prevent a power on, it should just decrement
    // the count to 0.  The quiesce handling is where the
    // check of this count will occur
    if (value != server::Host::Transition::Off)
    {
        decrementRebootCount();
    }

    executeTransition(value);

    auto retVal = server::Host::requestedHostTransition(value);
    serialize();
    return retVal;
}

Host::ProgressStages Host::bootProgress(ProgressStages value)
{
    auto retVal = bootprogress::Progress::bootProgress(value);
    serialize();
    return retVal;
}

Host::OSStatus Host::operatingSystemState(OSStatus value)
{
    auto retVal = osstatus::Status::operatingSystemState(value);
    serialize();
    return retVal;
}

Host::HostState Host::currentHostState(HostState value)
{
    log<level::INFO>(
        "Change to Host State",
        entry("CURRENT_HOST_STATE=%s", convertForMessage(value).c_str()));
    return server::Host::currentHostState(value);
}

} // namespace manager
} // namespace state
} // namespace phosphor
