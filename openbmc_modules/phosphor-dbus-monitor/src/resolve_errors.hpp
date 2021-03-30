#pragma once
#include "callback.hpp"

#include <string>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/**
 * @class ResolveCallout
 * @brief Resolves error logs with the associated callout
 *
 * Resolves a log by setting its Resolved property
 * to true.
 */
class ResolveCallout : public Callback
{
  public:
    ResolveCallout() = delete;
    ~ResolveCallout() = default;
    ResolveCallout(const ResolveCallout&) = delete;
    ResolveCallout& operator=(const ResolveCallout&) = delete;
    ResolveCallout(ResolveCallout&&) = default;
    ResolveCallout& operator=(ResolveCallout&&) = default;

    /**
     * @brief constructor
     *
     * @param[in] callout - The callout whose errors need to be resolved.
     *                      Normally an inventory path.
     */
    explicit ResolveCallout(const std::string& callout) : callout(callout)
    {
    }

    /**
     * @brief Callback interface to resolve errors
     *
     * Resolves all error log entries that are associated
     * with the callout.
     */
    void operator()(Context ctx) override;

  private:
    /**
     * @brief Resolves a single error log entry
     *
     * param[in] entry - the object path of the error log entry
     */
    void resolve(const std::string& entry);

    /**
     * @brief The object path of the callout, typically an inventory path
     */
    std::string callout;
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
