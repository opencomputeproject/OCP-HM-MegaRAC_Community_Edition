#pragma once

#include "message_handler.hpp"

#include <ipmid/api.h>

#include <cstddef>
#include <functional>
#include <map>

namespace command
{

struct CommandID
{
    static constexpr size_t lunBits = 2;
    CommandID(uint32_t command) : command(command)
    {
    }

    uint8_t netFnLun() const
    {
        return static_cast<uint8_t>(command >> CHAR_BIT);
    }
    uint8_t netFn() const
    {
        return netFnLun() >> lunBits;
    }
    uint8_t lun() const
    {
        return netFnLun() & ((1 << (lunBits + 1)) - 1);
    }
    uint8_t cmd() const
    {
        return static_cast<uint8_t>(command);
    }
    uint32_t command;
};

/**
 * CommandFunctor is the functor register for commands defined in
 * phosphor-net-ipmid. This would take the request part of the command as a
 * vector and a reference to the message handler. The response part of the
 * command is returned as a vector.
 */
using CommandFunctor = std::function<std::vector<uint8_t>(
    const std::vector<uint8_t>&, const message::Handler&)>;

/**
 * @struct CmdDetails
 *
 * Command details is used to register commands supported in phosphor-net-ipmid.
 */
struct CmdDetails
{
    CommandID command;
    CommandFunctor functor;
    session::Privilege privilege;
    bool sessionless;
};

/**
 * @enum NetFns
 *
 * A field that identifies the functional class of the message. The Network
 * Function clusters IPMI commands into different sets.
 */
enum class NetFns
{
    CHASSIS = (0x00 << 10),
    CHASSIS_RESP = (0x01 << 10),

    BRIDGE = (0x02 << 10),
    BRIDGE_RESP = (0x03 << 10),

    SENSOR = (0x04 << 10),
    SENSOR_RESP = (0x05 << 10),
    EVENT = (0x04 << 10),
    EVENT_RESP = (0x05 << 10),

    APP = (0x06 << 10),
    APP_RESP = (0x07 << 10),

    FIRMWARE = (0x08 << 10),
    FIRMWARE_RESP = (0x09 << 10),

    STORAGE = (0x0A << 10),
    STORAGE_RESP = (0x0B << 10),

    TRANSPORT = (0x0C << 10),
    TRANSPORT_RESP = (0x0D << 10),

    //>>
    RESERVED_START = (0x0E << 10),
    RESERVED_END = (0x2B << 10),
    //<<

    GROUP_EXTN = (0x2C << 10),
    GROUP_EXTN_RESP = (0x2D << 10),

    OEM = (0x2E << 10),
    OEM_RESP = (0x2F << 10),
};

/**
 * @class Entry
 *
 * This is the base class for registering IPMI commands. There are two ways of
 * registering commands to phosphor-net-ipmid, the session related commands and
 * provider commands
 *
 * Every commands has a privilege level which mentions the minimum session
 * privilege level needed to execute the command
 */

class Entry
{

  public:
    Entry(CommandID command, session::Privilege privilege) :
        command(command), privilege(privilege)
    {
    }

    /**
     * @brief Execute the command
     *
     * Execute the command
     *
     * @param[in] commandData - Request Data for the command
     * @param[in] handler - Reference to the Message Handler
     *
     * @return Response data for the command
     */
    virtual std::vector<uint8_t>
        executeCommand(std::vector<uint8_t>& commandData,
                       std::shared_ptr<message::Handler> handler) = 0;

    auto getCommand() const
    {
        return command;
    }

    auto getPrivilege() const
    {
        return privilege;
    }

    virtual ~Entry() = default;
    Entry(const Entry&) = default;
    Entry& operator=(const Entry&) = default;
    Entry(Entry&&) = default;
    Entry& operator=(Entry&&) = default;

  protected:
    CommandID command;

    // Specifies the minimum privilege level required to execute this command
    session::Privilege privilege;
};

/**
 * @class NetIpmidEntry
 *
 * NetIpmidEntry is used to register commands that are consumed only in
 * phosphor-net-ipmid. The RAKP commands, session commands and user management
 * commands are examples of this.
 *
 * There are certain IPMI commands that can be executed before session can be
 * established like Get System GUID, Get Channel Authentication Capabilities
 * and RAKP commands.
 */
class NetIpmidEntry final : public Entry
{

  public:
    NetIpmidEntry(CommandID command, CommandFunctor functor,
                  session::Privilege privilege, bool sessionless) :
        Entry(command, privilege),
        functor(functor), sessionless(sessionless)
    {
    }

    /**
     * @brief Execute the command
     *
     * Execute the command
     *
     * @param[in] commandData - Request Data for the command
     * @param[in] handler - Reference to the Message Handler
     *
     * @return Response data for the command
     */
    std::vector<uint8_t>
        executeCommand(std::vector<uint8_t>& commandData,
                       std::shared_ptr<message::Handler> handler) override;

    virtual ~NetIpmidEntry() = default;
    NetIpmidEntry(const NetIpmidEntry&) = default;
    NetIpmidEntry& operator=(const NetIpmidEntry&) = default;
    NetIpmidEntry(NetIpmidEntry&&) = default;
    NetIpmidEntry& operator=(NetIpmidEntry&&) = default;

  private:
    CommandFunctor functor;

    bool sessionless;
};

/**
 * @class Table
 *
 * Table keeps the IPMI command entries as a sorted associative container with
 * Command ID as the unique key. It has interfaces for registering commands
 * and executing a command.
 */
class Table
{
  public:
    Table() = default;
    ~Table() = default;
    // Command Table is a singleton so copy, copy-assignment, move and
    // move assignment is deleted
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    Table(Table&&) = default;
    Table& operator=(Table&&) = default;

    using CommandTable = std::map<uint32_t, std::unique_ptr<Entry>>;

    /**
     * @brief Register a command
     *
     * Register a command with the command table
     *
     * @param[in] inCommand - Command ID
     * @param[in] entry - Command Entry
     *
     * @return: None
     *
     * @note: Duplicate registrations will be rejected.
     *
     */
    void registerCommand(CommandID inCommand, std::unique_ptr<Entry>&& entry);

    /**
     * @brief Execute the command
     *
     * Execute the command for the corresponding CommandID
     *
     * @param[in] inCommand - Command ID to execute.
     * @param[in] commandData - Request Data for the command
     * @param[in] handler - Reference to the Message Handler
     *
     */
    void executeCommand(uint32_t inCommand, std::vector<uint8_t>& commandData,
                        std::shared_ptr<message::Handler> handler);

  private:
    CommandTable commandTable;
};

} // namespace command
