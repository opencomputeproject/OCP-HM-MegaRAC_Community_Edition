#pragma once
#include "callback.hpp"

namespace phosphor
{
namespace dbus
{
namespace monitoring
{
/** @class Trap
 *  @brief Raises SNMP trap
 */
class Trap
{
  public:
    Trap() = default;
    Trap(const Trap&) = delete;
    Trap(Trap&&) = default;
    Trap& operator=(const Trap&) = delete;
    Trap& operator=(Trap&&) = default;
    virtual ~Trap() = default;
    /** @brief Raise SNMP trap by parsing the sdbus message.
     *  @param[in] msg - sdbus message.
     */
    virtual void trap(sdbusplus::message::message& msg) const = 0;
};

/** @class ErrorTrap
 *  @brief Sends SNMP trap for the elog error
 */
class ErrorTrap : public Trap
{
  public:
    ErrorTrap() = default;
    ErrorTrap(const ErrorTrap&) = delete;
    ErrorTrap(ErrorTrap&&) = default;
    ErrorTrap& operator=(const ErrorTrap&) = delete;
    ErrorTrap& operator=(ErrorTrap&&) = default;
    ~ErrorTrap() = default;

    /** @brief Raise SNMP trap by parsing the sdbus message.
     *  @param[in] msg - sdbus message.
     */
    void trap(sdbusplus::message::message& msg) const override;
};

/** @class SNMPTrap
 *  @brief SNMP trap callback implementation.
 */
template <typename T>
class SNMPTrap : public Callback
{
  public:
    SNMPTrap(const SNMPTrap&) = delete;
    SNMPTrap(SNMPTrap&&) = default;
    SNMPTrap& operator=(const SNMPTrap&) = delete;
    SNMPTrap& operator=(SNMPTrap&&) = default;
    virtual ~SNMPTrap() = default;
    SNMPTrap() : Callback()
    {
    }

    /** @brief Callback interface implementation.
     *  @param[in] ctc - context.
     */
    void operator()(Context ctx)
    {
    }

    /** @brief Callback interface implementation.
     *  @param[in] ctc - context.
     *  @param[in] msg - sdbus message.
     */
    void operator()(Context ctx, sdbusplus::message::message& msg)
    {
        event.trap(msg);
    }

  private:
    T event;
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
