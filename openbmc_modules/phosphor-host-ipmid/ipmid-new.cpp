/**
 * Copyright © 2018 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "config.h"

#include "settings.hpp"

#include <dlfcn.h>

#include <algorithm>
#include <any>
#include <boost/algorithm/string.hpp>
#include <dcmihandler.hpp>
#include <exception>
#include <filesystem>
#include <forward_list>
#include <host-cmd-manager.hpp>
#include <ipmid-host/cmd.hpp>
#include <ipmid/api.hpp>
#include <ipmid/handler.hpp>
#include <ipmid/message.hpp>
#include <ipmid/oemrouter.hpp>
#include <ipmid/types.hpp>
#include <map>
#include <memory>
#include <optional>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/timer.hpp>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using namespace phosphor::logging;

// IPMI Spec, shared Reservation ID.
static unsigned short selReservationID = 0xFFFF;
static bool selReservationValid = false;

unsigned short reserveSel(void)
{
    // IPMI spec, Reservation ID, the value simply increases against each
    // execution of the Reserve SEL command.
    if (++selReservationID == 0)
    {
        selReservationID = 1;
    }
    selReservationValid = true;
    return selReservationID;
}

bool checkSELReservation(unsigned short id)
{
    return (selReservationValid && selReservationID == id);
}

void cancelSELReservation(void)
{
    selReservationValid = false;
}

EInterfaceIndex getInterfaceIndex(void)
{
    return interfaceKCS;
}

sd_bus* bus;
sd_event* events = nullptr;
sd_event* ipmid_get_sd_event_connection(void)
{
    return events;
}
sd_bus* ipmid_get_sd_bus_connection(void)
{
    return bus;
}

namespace ipmi
{

static inline unsigned int makeCmdKey(unsigned int cluster, unsigned int cmd)
{
    return (cluster << 8) | cmd;
}

using HandlerTuple = std::tuple<int,                        /* prio */
                                Privilege, HandlerBase::ptr /* handler */
                                >;

/* map to handle standard registered commands */
static std::unordered_map<unsigned int, /* key is NetFn/Cmd */
                          HandlerTuple>
    handlerMap;

/* special map for decoding Group registered commands (NetFn 2Ch) */
static std::unordered_map<unsigned int, /* key is Group/Cmd (NetFn is 2Ch) */
                          HandlerTuple>
    groupHandlerMap;

/* special map for decoding OEM registered commands (NetFn 2Eh) */
static std::unordered_map<unsigned int, /* key is Iana/Cmd (NetFn is 2Eh) */
                          HandlerTuple>
    oemHandlerMap;

using FilterTuple = std::tuple<int,            /* prio */
                               FilterBase::ptr /* filter */
                               >;

/* list to hold all registered ipmi command filters */
static std::forward_list<FilterTuple> filterList;

namespace impl
{
/* common function to register all standard IPMI handlers */
bool registerHandler(int prio, NetFn netFn, Cmd cmd, Privilege priv,
                     HandlerBase::ptr handler)
{
    // check for valid NetFn: even; 00-0Ch, 30-3Eh
    if (netFn & 1 || (netFn > netFnTransport && netFn < netFnGroup) ||
        netFn > netFnOemEight)
    {
        return false;
    }

    // create key and value for this handler
    unsigned int netFnCmd = makeCmdKey(netFn, cmd);
    HandlerTuple item(prio, priv, handler);

    // consult the handler map and look for a match
    auto& mapCmd = handlerMap[netFnCmd];
    if (!std::get<HandlerBase::ptr>(mapCmd) || std::get<int>(mapCmd) <= prio)
    {
        mapCmd = item;
        return true;
    }
    return false;
}

/* common function to register all Group IPMI handlers */
bool registerGroupHandler(int prio, Group group, Cmd cmd, Privilege priv,
                          HandlerBase::ptr handler)
{
    // create key and value for this handler
    unsigned int netFnCmd = makeCmdKey(group, cmd);
    HandlerTuple item(prio, priv, handler);

    // consult the handler map and look for a match
    auto& mapCmd = groupHandlerMap[netFnCmd];
    if (!std::get<HandlerBase::ptr>(mapCmd) || std::get<int>(mapCmd) <= prio)
    {
        mapCmd = item;
        return true;
    }
    return false;
}

/* common function to register all OEM IPMI handlers */
bool registerOemHandler(int prio, Iana iana, Cmd cmd, Privilege priv,
                        HandlerBase::ptr handler)
{
    // create key and value for this handler
    unsigned int netFnCmd = makeCmdKey(iana, cmd);
    HandlerTuple item(prio, priv, handler);

    // consult the handler map and look for a match
    auto& mapCmd = oemHandlerMap[netFnCmd];
    if (!std::get<HandlerBase::ptr>(mapCmd) || std::get<int>(mapCmd) <= prio)
    {
        mapCmd = item;
        return true;
    }
    return false;
}

/* common function to register all IPMI filter handlers */
void registerFilter(int prio, FilterBase::ptr filter)
{
    // check for initial placement
    if (filterList.empty() || std::get<int>(filterList.front()) < prio)
    {
        filterList.emplace_front(std::make_tuple(prio, filter));
    }
    // walk the list and put it in the right place
    auto j = filterList.begin();
    for (auto i = j; i != filterList.end() && std::get<int>(*i) > prio; i++)
    {
        j = i;
    }
    filterList.emplace_after(j, std::make_tuple(prio, filter));
}

} // namespace impl

message::Response::ptr filterIpmiCommand(message::Request::ptr request)
{
    // pass the command through the filter mechanism
    // This can be the firmware firewall or any OEM mechanism like
    // whitelist filtering based on operational mode
    for (auto& item : filterList)
    {
        FilterBase::ptr filter = std::get<FilterBase::ptr>(item);
        ipmi::Cc cc = filter->call(request);
        if (ipmi::ccSuccess != cc)
        {
            return errorResponse(request, cc);
        }
    }
    return message::Response::ptr();
}

message::Response::ptr executeIpmiCommandCommon(
    std::unordered_map<unsigned int, HandlerTuple>& handlers,
    unsigned int keyCommon, message::Request::ptr request)
{
    // filter the command first; a non-null message::Response::ptr
    // means that the message has been rejected for some reason
    message::Response::ptr filterResponse = filterIpmiCommand(request);

    Cmd cmd = request->ctx->cmd;
    unsigned int key = makeCmdKey(keyCommon, cmd);
    auto cmdIter = handlers.find(key);
    if (cmdIter != handlers.end())
    {
        // only return the filter response if the command is found
        if (filterResponse)
        {
            return filterResponse;
        }
        HandlerTuple& chosen = cmdIter->second;
        if (request->ctx->priv < std::get<Privilege>(chosen))
        {
            return errorResponse(request, ccInsufficientPrivilege);
        }
        return std::get<HandlerBase::ptr>(chosen)->call(request);
    }
    else
    {
        unsigned int wildcard = makeCmdKey(keyCommon, cmdWildcard);
        cmdIter = handlers.find(wildcard);
        if (cmdIter != handlers.end())
        {
            // only return the filter response if the command is found
            if (filterResponse)
            {
                return filterResponse;
            }
            HandlerTuple& chosen = cmdIter->second;
            if (request->ctx->priv < std::get<Privilege>(chosen))
            {
                return errorResponse(request, ccInsufficientPrivilege);
            }
            return std::get<HandlerBase::ptr>(chosen)->call(request);
        }
    }
    return errorResponse(request, ccInvalidCommand);
}

message::Response::ptr executeIpmiGroupCommand(message::Request::ptr request)
{
    // look up the group for this request
    uint8_t bytes;
    if (0 != request->payload.unpack(bytes))
    {
        return errorResponse(request, ccReqDataLenInvalid);
    }
    auto group = static_cast<Group>(bytes);
    message::Response::ptr response =
        executeIpmiCommandCommon(groupHandlerMap, group, request);
    ipmi::message::Payload prefix;
    prefix.pack(bytes);
    response->prepend(prefix);
    return response;
}

message::Response::ptr executeIpmiOemCommand(message::Request::ptr request)
{
    // look up the iana for this request
    uint24_t bytes;
    if (0 != request->payload.unpack(bytes))
    {
        return errorResponse(request, ccReqDataLenInvalid);
    }
    auto iana = static_cast<Iana>(bytes);
    message::Response::ptr response =
        executeIpmiCommandCommon(oemHandlerMap, iana, request);
    ipmi::message::Payload prefix;
    prefix.pack(bytes);
    response->prepend(prefix);
    return response;
}

message::Response::ptr executeIpmiCommand(message::Request::ptr request)
{
    NetFn netFn = request->ctx->netFn;
    if (netFnGroup == netFn)
    {
        return executeIpmiGroupCommand(request);
    }
    else if (netFnOem == netFn)
    {
        return executeIpmiOemCommand(request);
    }
    return executeIpmiCommandCommon(handlerMap, netFn, request);
}

namespace utils
{
template <typename AssocContainer, typename UnaryPredicate>
void assoc_erase_if(AssocContainer& c, UnaryPredicate p)
{
    typename AssocContainer::iterator next = c.begin();
    typename AssocContainer::iterator last = c.end();
    while ((next = std::find_if(next, last, p)) != last)
    {
        c.erase(next++);
    }
}
} // namespace utils

namespace
{
std::unordered_map<std::string, uint8_t> uniqueNameToChannelNumber;

// sdbusplus::bus::match::rules::arg0namespace() wants the prefix
// to match without any trailing '.'
constexpr const char ipmiDbusChannelMatch[] =
    "xyz.openbmc_project.Ipmi.Channel";
void updateOwners(sdbusplus::asio::connection& conn, const std::string& name)
{
    conn.async_method_call(
        [name](const boost::system::error_code ec,
               const std::string& nameOwner) {
            if (ec)
            {
                log<level::ERR>("Error getting dbus owner",
                                entry("INTERFACE=%s", name.c_str()));
                return;
            }
            // start after ipmiDbusChannelPrefix (after the '.')
            std::string chName =
                name.substr(std::strlen(ipmiDbusChannelMatch) + 1);
            try
            {
                uint8_t channel = getChannelByName(chName);
                uniqueNameToChannelNumber[nameOwner] = channel;
                log<level::INFO>("New interface mapping",
                                 entry("INTERFACE=%s", name.c_str()),
                                 entry("CHANNEL=%u", channel));
            }
            catch (const std::exception& e)
            {
                log<level::INFO>("Failed interface mapping, no such name",
                                 entry("INTERFACE=%s", name.c_str()));
            }
        },
        "org.freedesktop.DBus", "/", "org.freedesktop.DBus", "GetNameOwner",
        name);
}

void doListNames(boost::asio::io_service& io, sdbusplus::asio::connection& conn)
{
    conn.async_method_call(
        [&io, &conn](const boost::system::error_code ec,
                     std::vector<std::string> busNames) {
            if (ec)
            {
                log<level::ERR>("Error getting dbus names");
                std::exit(EXIT_FAILURE);
                return;
            }
            // Try to make startup consistent
            std::sort(busNames.begin(), busNames.end());

            const std::string channelPrefix =
                std::string(ipmiDbusChannelMatch) + ".";
            for (const std::string& busName : busNames)
            {
                if (busName.find(channelPrefix) == 0)
                {
                    updateOwners(conn, busName);
                }
            }
        },
        "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
        "ListNames");
}

void nameChangeHandler(sdbusplus::message::message& message)
{
    std::string name;
    std::string oldOwner;
    std::string newOwner;

    message.read(name, oldOwner, newOwner);

    if (!oldOwner.empty())
    {
        if (boost::starts_with(oldOwner, ":"))
        {
            // Connection removed
            auto it = uniqueNameToChannelNumber.find(oldOwner);
            if (it != uniqueNameToChannelNumber.end())
            {
                uniqueNameToChannelNumber.erase(it);
            }
        }
    }
    if (!newOwner.empty())
    {
        // start after ipmiDbusChannelMatch (and after the '.')
        std::string chName = name.substr(std::strlen(ipmiDbusChannelMatch) + 1);
        try
        {
            uint8_t channel = getChannelByName(chName);
            uniqueNameToChannelNumber[newOwner] = channel;
            log<level::INFO>("New interface mapping",
                             entry("INTERFACE=%s", name.c_str()),
                             entry("CHANNEL=%u", channel));
        }
        catch (const std::exception& e)
        {
            log<level::INFO>("Failed interface mapping, no such name",
                             entry("INTERFACE=%s", name.c_str()));
        }
    }
};

} // anonymous namespace

static constexpr const char intraBmcName[] = "INTRABMC";
uint8_t channelFromMessage(sdbusplus::message::message& msg)
{
    // channel name for ipmitool to resolve to
    std::string sender = msg.get_sender();
    auto chIter = uniqueNameToChannelNumber.find(sender);
    if (chIter != uniqueNameToChannelNumber.end())
    {
        return chIter->second;
    }
    // FIXME: currently internal connections are ephemeral and hard to pin down
    try
    {
        return getChannelByName(intraBmcName);
    }
    catch (const std::exception& e)
    {
        return invalidChannel;
    }
} // namespace ipmi

/* called from sdbus async server context */
auto executionEntry(boost::asio::yield_context yield,
                    sdbusplus::message::message& m, NetFn netFn, uint8_t lun,
                    Cmd cmd, std::vector<uint8_t>& data,
                    std::map<std::string, ipmi::Value>& options)
{
    const auto dbusResponse =
        [netFn, lun, cmd](Cc cc, const std::vector<uint8_t>& data = {}) {
            constexpr uint8_t netFnResponse = 0x01;
            uint8_t retNetFn = netFn | netFnResponse;
            return std::make_tuple(retNetFn, lun, cmd, cc, data);
        };
    std::string sender = m.get_sender();
    Privilege privilege = Privilege::None;
    int rqSA = 0;
    uint8_t userId = 0; // undefined user
    uint32_t sessionId = 0;

    // figure out what channel the request came in on
    uint8_t channel = channelFromMessage(m);
    if (channel == invalidChannel)
    {
        // unknown sender channel; refuse to service the request
        log<level::ERR>("ERROR determining source IPMI channel",
                        entry("SENDER=%s", sender.c_str()),
                        entry("NETFN=0x%X", netFn), entry("CMD=0x%X", cmd));
        return dbusResponse(ipmi::ccDestinationUnavailable);
    }

    // session-based channels are required to provide userId, privilege and
    // sessionId
    if (getChannelSessionSupport(channel) != EChannelSessSupported::none)
    {
        try
        {
            Value requestPriv = options.at("privilege");
            Value requestUserId = options.at("userId");
            Value requestSessionId = options.at("currentSessionId");
            privilege = static_cast<Privilege>(std::get<int>(requestPriv));
            userId = static_cast<uint8_t>(std::get<int>(requestUserId));
            sessionId =
                static_cast<uint32_t>(std::get<uint32_t>(requestSessionId));
        }
        catch (const std::exception& e)
        {
            log<level::ERR>("ERROR determining IPMI session credentials",
                            entry("CHANNEL=%u", channel),
                            entry("NETFN=0x%X", netFn), entry("CMD=0x%X", cmd));
            return dbusResponse(ipmi::ccUnspecifiedError);
        }
    }
    else
    {
        // get max privilege for session-less channels
        // For now, there is not a way to configure this, default to Admin
        privilege = Privilege::Admin;

        // ipmb should supply rqSA
        ChannelInfo chInfo;
        getChannelInfo(channel, chInfo);
        if (static_cast<EChannelMediumType>(chInfo.mediumType) ==
            EChannelMediumType::ipmb)
        {
            const auto iter = options.find("rqSA");
            if (iter != options.end())
            {
                if (std::holds_alternative<int>(iter->second))
                {
                    rqSA = std::get<int>(iter->second);
                }
            }
        }
    }
    // check to see if the requested priv/username is valid
    log<level::DEBUG>("Set up ipmi context", entry("SENDER=%s", sender.c_str()),
                      entry("NETFN=0x%X", netFn), entry("CMD=0x%X", cmd),
                      entry("CHANNEL=%u", channel), entry("USERID=%u", userId),
                      entry("SESSIONID=0x%X", sessionId),
                      entry("PRIVILEGE=%u", static_cast<uint8_t>(privilege)),
                      entry("RQSA=%x", rqSA));

    auto ctx =
        std::make_shared<ipmi::Context>(getSdBus(), netFn, cmd, channel, userId,
                                        sessionId, privilege, rqSA, yield);
    auto request = std::make_shared<ipmi::message::Request>(
        ctx, std::forward<std::vector<uint8_t>>(data));
    message::Response::ptr response = executeIpmiCommand(request);

    return dbusResponse(response->cc, response->payload.raw);
}

/** @struct IpmiProvider
 *
 *  RAII wrapper for dlopen so that dlclose gets called on exit
 */
struct IpmiProvider
{
  public:
    /** @brief address of the opened library */
    void* addr;
    std::string name;

    IpmiProvider() = delete;
    IpmiProvider(const IpmiProvider&) = delete;
    IpmiProvider& operator=(const IpmiProvider&) = delete;
    IpmiProvider(IpmiProvider&&) = delete;
    IpmiProvider& operator=(IpmiProvider&&) = delete;

    /** @brief dlopen a shared object file by path
     *  @param[in]  filename - path of shared object to open
     */
    explicit IpmiProvider(const char* fname) : addr(nullptr), name(fname)
    {
        log<level::DEBUG>("Open IPMI provider library",
                          entry("PROVIDER=%s", name.c_str()));
        try
        {
            addr = dlopen(name.c_str(), RTLD_NOW);
        }
        catch (std::exception& e)
        {
            log<level::ERR>("ERROR opening IPMI provider",
                            entry("PROVIDER=%s", name.c_str()),
                            entry("ERROR=%s", e.what()));
        }
        catch (...)
        {
            std::exception_ptr eptr = std::current_exception();
            try
            {
                std::rethrow_exception(eptr);
            }
            catch (std::exception& e)
            {
                log<level::ERR>("ERROR opening IPMI provider",
                                entry("PROVIDER=%s", name.c_str()),
                                entry("ERROR=%s", e.what()));
            }
        }
        if (!isOpen())
        {
            log<level::ERR>("ERROR opening IPMI provider",
                            entry("PROVIDER=%s", name.c_str()),
                            entry("ERROR=%s", dlerror()));
        }
    }

    ~IpmiProvider()
    {
        if (isOpen())
        {
            dlclose(addr);
        }
    }
    bool isOpen() const
    {
        return (nullptr != addr);
    }
};

// Plugin libraries need to contain .so either at the end or in the middle
constexpr const char ipmiPluginExtn[] = ".so";

/* return a list of self-closing library handles */
std::forward_list<IpmiProvider> loadProviders(const fs::path& ipmiLibsPath)
{
    std::vector<fs::path> libs;
    for (const auto& libPath : fs::directory_iterator(ipmiLibsPath))
    {
        std::error_code ec;
        fs::path fname = libPath.path();
        if (fs::is_symlink(fname, ec) || ec)
        {
            // it's a symlink or some other error; skip it
            continue;
        }
        while (fname.has_extension())
        {
            fs::path extn = fname.extension();
            if (extn == ipmiPluginExtn)
            {
                libs.push_back(libPath.path());
                break;
            }
            fname.replace_extension();
        }
    }
    std::sort(libs.begin(), libs.end());

    std::forward_list<IpmiProvider> handles;
    for (auto& lib : libs)
    {
#ifdef __IPMI_DEBUG__
        log<level::DEBUG>("Registering handler",
                          entry("HANDLER=%s", lib.c_str()));
#endif
        handles.emplace_front(lib.c_str());
    }
    return handles;
}

} // namespace ipmi

#ifdef ALLOW_DEPRECATED_API
/* legacy registration */
void ipmi_register_callback(ipmi_netfn_t netFn, ipmi_cmd_t cmd,
                            ipmi_context_t context, ipmid_callback_t handler,
                            ipmi_cmd_privilege_t priv)
{
    auto h = ipmi::makeLegacyHandler(handler, context);
    // translate priv from deprecated enum to current
    ipmi::Privilege realPriv;
    switch (priv)
    {
        case PRIVILEGE_CALLBACK:
            realPriv = ipmi::Privilege::Callback;
            break;
        case PRIVILEGE_USER:
            realPriv = ipmi::Privilege::User;
            break;
        case PRIVILEGE_OPERATOR:
            realPriv = ipmi::Privilege::Operator;
            break;
        case PRIVILEGE_ADMIN:
            realPriv = ipmi::Privilege::Admin;
            break;
        case PRIVILEGE_OEM:
            realPriv = ipmi::Privilege::Oem;
            break;
        case SYSTEM_INTERFACE:
            realPriv = ipmi::Privilege::Admin;
            break;
        default:
            realPriv = ipmi::Privilege::Admin;
            break;
    }
    // The original ipmi_register_callback allowed for group OEM handlers
    // to be registered via this same interface. It just so happened that
    // all the handlers were part of the DCMI group, so default to that.
    if (netFn == NETFUN_GRPEXT)
    {
        ipmi::impl::registerGroupHandler(ipmi::prioOpenBmcBase,
                                         dcmi::groupExtId, cmd, realPriv, h);
    }
    else
    {
        ipmi::impl::registerHandler(ipmi::prioOpenBmcBase, netFn, cmd, realPriv,
                                    h);
    }
}

namespace oem
{

class LegacyRouter : public oem::Router
{
  public:
    virtual ~LegacyRouter()
    {
    }

    /// Enable message routing to begin.
    void activate() override
    {
    }

    void registerHandler(Number oen, ipmi_cmd_t cmd, Handler handler) override
    {
        auto h = ipmi::makeLegacyHandler(std::forward<Handler>(handler));
        ipmi::impl::registerOemHandler(ipmi::prioOpenBmcBase, oen, cmd,
                                       ipmi::Privilege::Admin, h);
    }
};
static LegacyRouter legacyRouter;

Router* mutableRouter()
{
    return &legacyRouter;
}

} // namespace oem

/* legacy alternative to executionEntry */
void handleLegacyIpmiCommand(sdbusplus::message::message& m)
{
    // make a copy so the next two moves don't wreak havoc on the stack
    sdbusplus::message::message b{m};
    boost::asio::spawn(*getIoContext(), [b = std::move(b)](
                                            boost::asio::yield_context yield) {
        sdbusplus::message::message m{std::move(b)};
        unsigned char seq, netFn, lun, cmd;
        std::vector<uint8_t> data;

        m.read(seq, netFn, lun, cmd, data);
        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
        auto ctx = std::make_shared<ipmi::Context>(
            bus, netFn, cmd, 0, 0, 0, ipmi::Privilege::Admin, 0, yield);
        auto request = std::make_shared<ipmi::message::Request>(
            ctx, std::forward<std::vector<uint8_t>>(data));
        ipmi::message::Response::ptr response =
            ipmi::executeIpmiCommand(request);

        // Responses in IPMI require a bit set.  So there ya go...
        netFn |= 0x01;

        const char *dest, *path;
        constexpr const char* DBUS_INTF = "org.openbmc.HostIpmi";

        dest = m.get_sender();
        path = m.get_path();
        boost::system::error_code ec;
        bus->yield_method_call(yield, ec, dest, path, DBUS_INTF, "sendMessage",
                               seq, netFn, lun, cmd, response->cc,
                               response->payload.raw);
        if (ec)
        {
            log<level::ERR>("Failed to send response to requestor",
                            entry("ERROR=%s", ec.message().c_str()),
                            entry("SENDER=%s", dest),
                            entry("NETFN=0x%X", netFn), entry("CMD=0x%X", cmd));
        }
    });
}

#endif /* ALLOW_DEPRECATED_API */

// Calls host command manager to do the right thing for the command
using CommandHandler = phosphor::host::command::CommandHandler;
std::unique_ptr<phosphor::host::command::Manager> cmdManager;
void ipmid_send_cmd_to_host(CommandHandler&& cmd)
{
    return cmdManager->execute(std::forward<CommandHandler>(cmd));
}

std::unique_ptr<phosphor::host::command::Manager>& ipmid_get_host_cmd_manager()
{
    return cmdManager;
}

// These are symbols that are present in libipmid, but not expected
// to be used except here (or maybe a unit test), so declare them here
extern void setIoContext(std::shared_ptr<boost::asio::io_context>& newIo);
extern void setSdBus(std::shared_ptr<sdbusplus::asio::connection>& newBus);

int main(int argc, char* argv[])
{
    // Connect to system bus
    auto io = std::make_shared<boost::asio::io_context>();
    setIoContext(io);
    if (argc > 1 && std::string(argv[1]) == "-session")
    {
        sd_bus_default_user(&bus);
    }
    else
    {
        sd_bus_default_system(&bus);
    }
    auto sdbusp = std::make_shared<sdbusplus::asio::connection>(*io, bus);
    setSdBus(sdbusp);

    // TODO: Hack to keep the sdEvents running.... Not sure why the sd_event
    //       queue stops running if we don't have a timer that keeps re-arming
    phosphor::Timer t2([]() { ; });
    t2.start(std::chrono::microseconds(500000), true);

    // TODO: Remove all vestiges of sd_event from phosphor-host-ipmid
    //       until that is done, add the sd_event wrapper to the io object
    sdbusplus::asio::sd_event_wrapper sdEvents(*io);

    cmdManager = std::make_unique<phosphor::host::command::Manager>(*sdbusp);

    // Register all command providers and filters
    std::forward_list<ipmi::IpmiProvider> providers =
        ipmi::loadProviders(HOST_IPMI_LIB_PATH);

#ifdef ALLOW_DEPRECATED_API
    // listen on deprecated signal interface for kcs/bt commands
    constexpr const char* FILTER = "type='signal',interface='org.openbmc."
                                   "HostIpmi',member='ReceivedMessage'";
    sdbusplus::bus::match::match oldIpmiInterface(*sdbusp, FILTER,
                                                  handleLegacyIpmiCommand);
#endif /* ALLOW_DEPRECATED_API */

    // set up bus name watching to match channels with bus names
    sdbusplus::bus::match::match nameOwnerChanged(
        *sdbusp,
        sdbusplus::bus::match::rules::nameOwnerChanged() +
            sdbusplus::bus::match::rules::arg0namespace(
                ipmi::ipmiDbusChannelMatch),
        ipmi::nameChangeHandler);
    ipmi::doListNames(*io, *sdbusp);

    int exitCode = 0;
    // set up boost::asio signal handling
    std::function<SignalResponse(int)> stopAsioRunLoop =
        [&io, &exitCode](int signalNumber) {
            log<level::INFO>("Received signal; quitting",
                             entry("SIGNAL=%d", signalNumber));
            io->stop();
            exitCode = signalNumber;
            return SignalResponse::breakExecution;
        };
    registerSignalHandler(ipmi::prioOpenBmcBase, SIGINT, stopAsioRunLoop);
    registerSignalHandler(ipmi::prioOpenBmcBase, SIGTERM, stopAsioRunLoop);

    sdbusp->request_name("xyz.openbmc_project.Ipmi.Host");
    // Add bindings for inbound IPMI requests
    auto server = sdbusplus::asio::object_server(sdbusp);
    auto iface = server.add_interface("/xyz/openbmc_project/Ipmi",
                                      "xyz.openbmc_project.Ipmi.Server");
    iface->register_method("execute", ipmi::executionEntry);
    iface->initialize();

    io->run();

    // destroy all the IPMI handlers so the providers can unload safely
    ipmi::handlerMap.clear();
    ipmi::groupHandlerMap.clear();
    ipmi::oemHandlerMap.clear();
    ipmi::filterList.clear();
    // unload the provider libraries
    providers.clear();

    std::exit(exitCode);
}
