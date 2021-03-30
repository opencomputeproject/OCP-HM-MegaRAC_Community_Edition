#pragma once

#include "callback.hpp"

#include <experimental/tuple>
#include <phosphor-logging/log.hpp>
#include <string>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{
namespace detail
{

using namespace phosphor::logging;

/** @class CallDBusMethod
 *  @brief Provide explicit call forwarding to
 *     DBusInterface::callMethodNoReply.
 *
 *  @tparam DBusInterface - The DBus interface to use.
 *  @tparam MethodArgs - DBus method argument types.
 */
template <typename DBusInterface, typename... MethodArgs>
struct CallDBusMethod
{
    static void op(const std::string& bus, const std::string& path,
                   const std::string& iface, const std::string& method,
                   MethodArgs&&... args)
    {
        try
        {
            DBusInterface::callMethodNoReply(bus, path, iface, method,
                                             std::forward<MethodArgs>(args)...);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            // clang-format off
            log<level::ERR>("Unable to call DBus method",
                            entry("BUS=%s", bus.c_str(),
                                  "PATH=%s", path.c_str(),
                                  "IFACE=%s", iface.c_str(),
                                  "METHOD=%s", method.c_str(),
                                  "ERROR=%s", e.what()));
            // clang-format on
        }
    }
};
} // namespace detail

/** @class MethodBase
 *  @brief Invoke DBus method callback implementation.
 *
 *  The method callback invokes the client supplied DBus method.
 */
class MethodBase : public Callback
{
  public:
    MethodBase() = delete;
    MethodBase(const MethodBase&) = delete;
    MethodBase(MethodBase&&) = default;
    MethodBase& operator=(const MethodBase&) = delete;
    MethodBase& operator=(MethodBase&&) = default;
    virtual ~MethodBase() = default;
    MethodBase(const std::string& b, const std::string& p, const std::string& i,
               const std::string& m) :
        Callback(),
        bus(b), path(p), interface(i), method(m)
    {
    }

    /** @brief Callback interface implementation. */
    void operator()(Context ctx) override = 0;

  protected:
    const std::string& bus;
    const std::string& path;
    const std::string& interface;
    const std::string& method;
};

/** @class Method
 *  @brief C++ type specific logic for the method callback.
 *
 *  @tparam DBusInterface - The DBus interface to use to call the method.
 *  @tparam MethodArgs - DBus method argument types.
 */
template <typename DBusInterface, typename... MethodArgs>
class Method : public MethodBase
{
  public:
    Method() = delete;
    Method(const Method&) = default;
    Method(Method&&) = default;
    Method& operator=(const Method&) = default;
    Method& operator=(Method&&) = default;
    ~Method() = default;
    Method(const std::string& bus, const std::string& path,
           const std::string& iface, const std::string& method,
           MethodArgs&&... arguments) :
        MethodBase(bus, path, iface, method),
        args(std::forward<MethodArgs>(arguments)...)
    {
    }

    /** @brief Callback interface implementation. */
    void operator()(Context ctx) override
    {
        std::experimental::apply(
            detail::CallDBusMethod<DBusInterface, MethodArgs...>::op,
            std::tuple_cat(std::make_tuple(bus), std::make_tuple(path),
                           std::make_tuple(interface), std::make_tuple(method),
                           args));
    }

  private:
    std::tuple<MethodArgs...> args;
};

/** @brief Argument type deduction for constructing Method instances. */
template <typename DBusInterface, typename... MethodArgs>
auto makeMethod(const std::string& bus, const std::string& path,
                const std::string& iface, const std::string& method,
                MethodArgs&&... arguments)
{
    return std::make_unique<Method<DBusInterface, MethodArgs...>>(
        bus, path, iface, method, std::forward<MethodArgs>(arguments)...);
}

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
