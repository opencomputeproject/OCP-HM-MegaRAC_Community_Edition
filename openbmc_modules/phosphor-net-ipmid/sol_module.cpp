#include "command/payload_cmds.hpp"
#include "command/sol_cmds.hpp"
#include "command_table.hpp"
#include "main.hpp"
#include "session.hpp"

namespace sol
{

namespace command
{

void registerCommands()
{
    static const ::command::CmdDetails commands[] = {
        // SOL Payload Handler
        {{(static_cast<uint32_t>(message::PayloadType::SOL) << 16)},
         &payloadHandler,
         session::Privilege::HIGHEST_MATCHING,
         false},
        // Activate Payload Command
        {{(static_cast<uint32_t>(message::PayloadType::IPMI) << 16) |
          static_cast<uint16_t>(::command::NetFns::APP) | 0x48},
         &activatePayload,
         session::Privilege::USER,
         false},
        // Deactivate Payload Command
        {{(static_cast<uint32_t>(message::PayloadType::IPMI) << 16) |
          static_cast<uint16_t>(::command::NetFns::APP) | 0x49},
         &deactivatePayload,
         session::Privilege::USER,
         false},
        // Get Payload Activation Status
        {{(static_cast<uint32_t>(message::PayloadType::IPMI) << 16) |
          static_cast<uint16_t>(::command::NetFns::APP) | 0x4A},
         &getPayloadStatus,
         session::Privilege::USER,
         false},
        // Get Payload Instance Info Command
        {{(static_cast<uint32_t>(message::PayloadType::IPMI) << 16) |
          static_cast<uint16_t>(::command::NetFns::APP) | 0x4B},
         &getPayloadInfo,
         session::Privilege::USER,
         false},
        // Set SOL Configuration Parameters
        {{(static_cast<uint32_t>(message::PayloadType::IPMI) << 16) |
          static_cast<uint16_t>(::command::NetFns::TRANSPORT) | 0x21},
         &setConfParams,
         session::Privilege::ADMIN,
         false},
        // Get SOL Configuration Parameters
        {{(static_cast<uint32_t>(message::PayloadType::IPMI) << 16) |
          static_cast<uint16_t>(::command::NetFns::TRANSPORT) | 0x22},
         &getConfParams,
         session::Privilege::USER,
         false},
    };

    for (const auto& iter : commands)
    {
        std::get<::command::Table&>(singletonPool)
            .registerCommand(iter.command,
                             std::make_unique<::command::NetIpmidEntry>(
                                 iter.command, iter.functor, iter.privilege,
                                 iter.sessionless));
    }
}

} // namespace command

} // namespace sol
