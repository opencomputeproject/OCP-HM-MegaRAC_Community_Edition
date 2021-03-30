#include "globalhandler.hpp"

#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <string>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/State/BMC/server.hpp>

static constexpr auto bmcStateRoot = "/xyz/openbmc_project/state";
static constexpr auto bmcStateIntf = "xyz.openbmc_project.State.BMC";
static constexpr auto reqTransition = "RequestedBMCTransition";
static constexpr auto match = "bmc0";

using namespace phosphor::logging;
using BMC = sdbusplus::xyz::openbmc_project::State::server::BMC;

void register_netfn_global_functions() __attribute__((constructor));

void resetBMC()
{
    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};

    auto bmcStateObj =
        ipmi::getDbusObject(bus, bmcStateIntf, bmcStateRoot, match);

    auto service = ipmi::getService(bus, bmcStateIntf, bmcStateObj.first);

    ipmi::setDbusProperty(bus, service, bmcStateObj.first, bmcStateIntf,
                          reqTransition,
                          convertForMessage(BMC::Transition::Reboot));
}

/** @brief implements cold and warm reset commands
 *  @param - None
 *  @returns IPMI completion code.
 */
ipmi::RspType<> ipmiGlobalReset()
{
    try
    {
        resetBMC();
    }
    catch (std::exception& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }

    // Status code.
    return ipmi::responseSuccess();
}

void register_netfn_global_functions()
{

    // Cold Reset
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnApp,
                          ipmi::app::cmdColdReset, ipmi::Privilege::Admin,
                          ipmiGlobalReset);
    return;
}
