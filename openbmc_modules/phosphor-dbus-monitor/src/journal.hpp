#pragma once

#include "callback.hpp"
#include "format.hpp"

#include <phosphor-logging/log.hpp>
#include <string>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/** @class JournalBase
 *  @brief Journal callback implementation.
 *
 *  The journal callback logs the client message and
 *  journal metadata key value pairs as specified by the
 *  client supplied property index.
 */
class JournalBase : public IndexedCallback
{
  public:
    JournalBase() = delete;
    JournalBase(const JournalBase&) = delete;
    JournalBase(JournalBase&&) = default;
    JournalBase& operator=(const JournalBase&) = delete;
    JournalBase& operator=(JournalBase&&) = default;
    virtual ~JournalBase() = default;
    JournalBase(const char* msg, const PropertyIndex& index) :
        IndexedCallback(index), message(msg)
    {
    }

    /** @brief Callback interface implementation. */
    void operator()(Context ctx) override;

  private:
    /** @brief Delegate type specific calls to subclasses. */
    virtual void log(const char* message, const std::string& pathMeta,
                     const std::string& path, const std::string& propertyMeta,
                     const any_ns::any& value) const = 0;

    /** @brief The client provided message to be traced.  */
    const char* message;
};

/** @struct Display
 *  @brief Convert strings to const char*.
 */
namespace detail
{
template <typename T>
struct Display
{
    static auto op(T&& value)
    {
        return std::forward<T>(value);
    }
};

template <>
struct Display<std::string>
{
    static auto op(const std::string& value)
    {
        return value.c_str();
    }
};
} // namespace detail

/** @class Journal
 *  @brief C++ type specific logic for the journal callback.
 *
 *  @tparam T - The C++ type of the property values being traced.
 *  @tparam Severity - The log severity of the log entry.
 */
template <typename T, phosphor::logging::level Severity>
class Journal : public JournalBase
{
  public:
    Journal() = delete;
    Journal(const Journal&) = delete;
    Journal(Journal&&) = default;
    Journal& operator=(const Journal&) = delete;
    Journal& operator=(Journal&&) = default;
    ~Journal() = default;
    Journal(const char* msg, const PropertyIndex& index) :
        JournalBase(msg, index)
    {
    }

  private:
    /** @brief log interface implementation. */
    void log(const char* message, const std::string& pathMeta,
             const std::string& path, const std::string& propertyMeta,
             const any_ns::any& value) const override
    {
        phosphor::logging::log<Severity>(
            message,
            phosphor::logging::entry(
                (pathMeta + GetFormat<decltype(pathMeta)>::format).c_str(),
                path.c_str()),
            phosphor::logging::entry(
                (propertyMeta + GetFormat<T>::format).c_str(),
                detail::Display<T>::op(any_ns::any_cast<T>(value))));
    }
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
