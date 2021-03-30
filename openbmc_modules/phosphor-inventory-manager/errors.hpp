#pragma once

#include <stdexcept>

namespace phosphor
{
namespace inventory
{
namespace manager
{

// TODO: Use proper error generation techniques
// https://github.com/openbmc/openbmc/issues/1125

/** @class InterfaceError
 *  @brief Exception class for unrecognized interfaces.
 */
class InterfaceError final : public std::invalid_argument
{
  public:
    ~InterfaceError() = default;
    InterfaceError() = delete;
    InterfaceError(const InterfaceError&) = delete;
    InterfaceError(InterfaceError&&) = default;
    InterfaceError& operator=(const InterfaceError&) = delete;
    InterfaceError& operator=(InterfaceError&&) = default;

    /** @brief Construct an interface error.
     *
     *  @param[in] msg - The message to be returned by what().
     *  @param[in] iface - The failing interface name.
     */
    InterfaceError(const char* msg, const std::string& iface) :
        std::invalid_argument(msg), interface(iface)
    {
    }

    /** @brief Log the exception message to the systemd journal. */
    void log() const;

  private:
    std::string interface;
};

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
