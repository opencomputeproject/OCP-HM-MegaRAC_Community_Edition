#include <algorithm>
#include <array>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <ipmiwhitelist.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <settings.hpp>
#include <xyz/openbmc_project/Control/Security/RestrictionMode/server.hpp>

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

namespace ipmi
{

// put the filter provider in an unnamed namespace
namespace
{

/** @class WhitelistFilter
 *
 * Class that implements an IPMI message filter based
 * on incoming interface and a restriction mode setting
 */
class WhitelistFilter
{
  public:
    WhitelistFilter();
    ~WhitelistFilter() = default;
    WhitelistFilter(WhitelistFilter const&) = delete;
    WhitelistFilter(WhitelistFilter&&) = delete;
    WhitelistFilter& operator=(WhitelistFilter const&) = delete;
    WhitelistFilter& operator=(WhitelistFilter&&) = delete;

  private:
    void postInit();
    void cacheRestrictedMode();
    void handleRestrictedModeChange(sdbusplus::message::message& m);
    ipmi::Cc filterMessage(ipmi::message::Request::ptr request);

    bool restrictedMode = true;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::unique_ptr<settings::Objects> objects;
    std::unique_ptr<sdbusplus::bus::match::match> modeChangeMatch;

    static constexpr const char restrictionModeIntf[] =
        "xyz.openbmc_project.Control.Security.RestrictionMode";
};

WhitelistFilter::WhitelistFilter()
{
    bus = getSdBus();

    log<level::INFO>("Loading whitelist filter");

    ipmi::registerFilter(ipmi::prioOpenBmcBase,
                         [this](ipmi::message::Request::ptr request) {
                             return filterMessage(request);
                         });

    // wait until io->run is going to fetch RestrictionMode
    post_work([this]() { postInit(); });
}

void WhitelistFilter::cacheRestrictedMode()
{
    using namespace sdbusplus::xyz::openbmc_project::Control::Security::server;
    std::string restrictionModeSetting;
    std::string restrictionModeService;
    try
    {
        restrictionModeSetting = objects->map.at(restrictionModeIntf).at(0);
        restrictionModeService =
            objects->service(restrictionModeSetting, restrictionModeIntf);
    }
    catch (const std::out_of_range& e)
    {
        log<level::ERR>(
            "Could not look up restriction mode interface from cache");
        return;
    }
    bus->async_method_call(
        [this](boost::system::error_code ec, ipmi::Value v) {
            if (ec)
            {
                log<level::ERR>("Error in RestrictionMode Get");
                // Fail-safe to true.
                restrictedMode = true;
                return;
            }
            auto mode = std::get<std::string>(v);
            auto restrictionMode =
                RestrictionMode::convertModesFromString(mode);
            restrictedMode =
                (restrictionMode == RestrictionMode::Modes::Whitelist);
            log<level::INFO>((restrictedMode ? "Set restrictedMode = true"
                                             : "Set restrictedMode = false"));
        },
        restrictionModeService, restrictionModeSetting,
        "org.freedesktop.DBus.Properties", "Get", restrictionModeIntf,
        "RestrictionMode");
}

void WhitelistFilter::handleRestrictedModeChange(sdbusplus::message::message& m)
{
    using namespace sdbusplus::xyz::openbmc_project::Control::Security::server;
    std::string intf;
    std::vector<std::pair<std::string, ipmi::Value>> propertyList;
    m.read(intf, propertyList);
    for (const auto& property : propertyList)
    {
        if (property.first == "RestrictionMode")
        {
            RestrictionMode::Modes restrictionMode =
                RestrictionMode::convertModesFromString(
                    std::get<std::string>(property.second));
            restrictedMode =
                (restrictionMode == RestrictionMode::Modes::Whitelist);
            log<level::INFO>((restrictedMode
                                  ? "Updated restrictedMode = true"
                                  : "Updated restrictedMode = false"));
        }
    }
}

void WhitelistFilter::postInit()
{
    objects = std::make_unique<settings::Objects>(
        *bus, std::vector<settings::Interface>({restrictionModeIntf}));
    if (!objects)
    {
        log<level::ERR>(
            "Failed to create settings object; defaulting to restricted mode");
        return;
    }

    // Initialize restricted mode
    cacheRestrictedMode();
    // Wait for changes on Restricted mode
    std::string filterStr;
    try
    {
        filterStr = sdbusplus::bus::match::rules::propertiesChanged(
            objects->map.at(restrictionModeIntf).at(0), restrictionModeIntf);
    }
    catch (const std::out_of_range& e)
    {
        log<level::ERR>("Failed to determine restriction mode filter string");
        return;
    }
    modeChangeMatch = std::make_unique<sdbusplus::bus::match::match>(
        *bus, filterStr, [this](sdbusplus::message::message& m) {
            handleRestrictedModeChange(m);
        });
}

ipmi::Cc WhitelistFilter::filterMessage(ipmi::message::Request::ptr request)
{
    if (request->ctx->channel == ipmi::channelSystemIface && restrictedMode)
    {
        if (!std::binary_search(
                whitelist.cbegin(), whitelist.cend(),
                std::make_pair(request->ctx->netFn, request->ctx->cmd)))
        {
            log<level::ERR>("Net function not whitelisted",
                            entry("NETFN=0x%X", int(request->ctx->netFn)),
                            entry("CMD=0x%X", int(request->ctx->cmd)));
            return ipmi::ccInsufficientPrivilege;
        }
    }
    return ipmi::ccSuccess;
}

// instantiate the WhitelistFilter when this shared object is loaded
WhitelistFilter whitelistFilter;

} // namespace

} // namespace ipmi
