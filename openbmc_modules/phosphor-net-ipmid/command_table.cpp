#include "command_table.hpp"

#include "main.hpp"
#include "message_handler.hpp"
#include "message_parsers.hpp"
#include "sessions_manager.hpp"

#include <iomanip>
#include <main.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <user_channel/user_layer.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;

namespace ipmi
{
using Value = std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                           int64_t, uint64_t, double, std::string>;

} // namespace ipmi

namespace command
{

void Table::registerCommand(CommandID inCommand, std::unique_ptr<Entry>&& entry)
{
    auto& command = commandTable[inCommand.command];

    if (command)
    {
        log<level::DEBUG>(
            "Already Registered",
            phosphor::logging::entry("SKIPPED_ENTRY=0x%x", inCommand.command));
        return;
    }

    command = std::move(entry);
}

void Table::executeCommand(uint32_t inCommand,
                           std::vector<uint8_t>& commandData,
                           std::shared_ptr<message::Handler> handler)
{
    using namespace std::chrono_literals;

    auto iterator = commandTable.find(inCommand);

    if (iterator == commandTable.end())
    {
        CommandID command(inCommand);

        auto bus = getSdBus();
        // forward the request onto the main ipmi queue
        using IpmiDbusRspType = std::tuple<uint8_t, uint8_t, uint8_t, uint8_t,
                                           std::vector<uint8_t>>;
        uint8_t lun = command.lun();
        uint8_t netFn = command.netFn();
        uint8_t cmd = command.cmd();
        std::shared_ptr<session::Session> session =
            std::get<session::Manager&>(singletonPool)
                .getSession(handler->sessionID);
        std::map<std::string, ipmi::Value> options = {
            {"userId", ipmi::Value(static_cast<int>(
                           ipmi::ipmiUserGetUserId(session->userName)))},
            {"privilege",
             ipmi::Value(static_cast<int>(session->currentPrivilege()))},
            {"currentSessionId",
             ipmi::Value(static_cast<uint32_t>(session->getBMCSessionID()))},
        };
        bus->async_method_call(
            [handler, this](const boost::system::error_code& ec,
                            const IpmiDbusRspType& response) {
                if (!ec)
                {
                    const uint8_t& cc = std::get<3>(response);
                    const std::vector<uint8_t>& responseData =
                        std::get<4>(response);
                    std::vector<uint8_t> payload;
                    payload.reserve(1 + responseData.size());
                    payload.push_back(cc);
                    payload.insert(payload.end(), responseData.begin(),
                                   responseData.end());
                    handler->outPayload = std::move(payload);
                }
                else
                {
                    std::vector<uint8_t> payload;
                    payload.push_back(IPMI_CC_UNSPECIFIED_ERROR);
                    handler->outPayload = std::move(payload);
                }
            },
            "xyz.openbmc_project.Ipmi.Host", "/xyz/openbmc_project/Ipmi",
            "xyz.openbmc_project.Ipmi.Server", "execute", netFn, lun, cmd,
            commandData, options);
    }
    else
    {
        auto start = std::chrono::steady_clock::now();

        handler->outPayload =
            iterator->second->executeCommand(commandData, handler);

        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<size_t> elapsedSeconds =
            std::chrono::duration_cast<std::chrono::seconds>(end - start);

        // If command time execution time exceeds 2 seconds, log a time
        // exceeded message
        if (elapsedSeconds > 2s)
        {
            log<level::ERR>("IPMI command timed out",
                            entry("DELAY=%zu", elapsedSeconds.count()));
        }
    }
}

std::vector<uint8_t>
    NetIpmidEntry::executeCommand(std::vector<uint8_t>& commandData,
                                  std::shared_ptr<message::Handler> handler)
{
    std::vector<uint8_t> errResponse;

    // Check if the command qualifies to be run prior to establishing a session
    if (!sessionless && (handler->sessionID == session::sessionZero))
    {
        errResponse.resize(1);
        errResponse[0] = IPMI_CC_INSUFFICIENT_PRIVILEGE;
        log<level::INFO>("Table: Insufficient privilege for command",
                         entry("LUN=%x", command.lun()),
                         entry("NETFN=%x", command.netFn()),
                         entry("CMD=%x", command.cmd()));
        return errResponse;
    }

    return functor(commandData, *handler);
}

} // namespace command
