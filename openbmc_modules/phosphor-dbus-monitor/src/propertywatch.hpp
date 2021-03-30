/**
 * @file propertywatch.hpp
 * @brief PropertyWatch class declarations.
 *
 * In general class users should include propertywatchimpl.hpp instead to avoid
 * link failures.
 */
#pragma once

#include "data_types.hpp"
#include "filters.hpp"
#include "watch.hpp"

#include <string>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

class Callback;

/** @class PropertyWatch
 *  @brief Type agnostic, factored out logic for property watches.
 *
 *  A property watch maintains the state of one or more DBus properties
 *  as specified by the supplied index.
 */
template <typename DBusInterfaceType>
class PropertyWatch : public Watch
{
  public:
    PropertyWatch() = delete;
    PropertyWatch(const PropertyWatch&) = delete;
    PropertyWatch(PropertyWatch&&) = default;
    PropertyWatch& operator=(const PropertyWatch&) = delete;
    PropertyWatch& operator=(PropertyWatch&&) = default;
    virtual ~PropertyWatch() = default;
    PropertyWatch(const PropertyIndex& watchIndex,
                  Callback* callback = nullptr) :
        Watch(),
        index(watchIndex), cb(callback), alreadyRan(false)
    {
    }

    /** @brief Start the watch.
     *
     *  Watch start interface implementation for PropertyWatch.
     */
    void start() override;

    /** @brief Run the watch callback method.
     *
     *  Watch callback interface implementation for PropertyWatch.
     */
    void callback(Context ctx) override;

    /** @brief Update properties.
     *
     *  Subclasses to query the properties specified by the index
     *  and update the cache.
     *
     *  @param[in] busName - The busname hosting the interface to query.
     *  @param[in] path - The path of the interface to query.
     *  @param[in] interface - The interface to query.
     */
    virtual void updateProperties(const std::string& busName,
                                  const std::string& path,
                                  const std::string& interface) = 0;

    /** @brief Dbus signal callback for PropertiesChanged.
     *
     *  Subclasses to update the cache.
     *
     *  @param[in] message - The org.freedesktop.DBus.PropertiesChanged
     *               message.
     *  @param[in] path - The path associated with the message.
     *  @param[in] interface - The interface associated with the message.
     */
    virtual void propertiesChanged(sdbusplus::message::message&,
                                   const std::string& path,
                                   const std::string& interface) = 0;

    /** @brief Dbus signal callback for InterfacesAdded.
     *
     *  Subclasses to update the cache.
     *
     *  @param[in] msg - The org.freedesktop.DBus.PropertiesChanged
     *               message.
     */
    virtual void interfacesAdded(sdbusplus::message::message& msg) = 0;

  protected:
    /** @brief Property names and their associated storage. */
    const PropertyIndex& index;

    /** @brief Optional callback method. */
    Callback* const cb;

    /** @brief The start method should only be invoked once. */
    bool alreadyRan;
};

/** @class PropertyWatchOfType
 *  @brief Type specific logic for PropertyWatch.
 *
 *  @tparam DBusInterfaceType - DBus access delegate.
 *  @tparam T - The type of the properties being watched.
 */
template <typename T, typename DBusInterfaceType>
class PropertyWatchOfType : public PropertyWatch<DBusInterfaceType>
{
  public:
    PropertyWatchOfType() = default;
    PropertyWatchOfType(const PropertyWatchOfType&) = delete;
    PropertyWatchOfType(PropertyWatchOfType&&) = default;
    PropertyWatchOfType& operator=(const PropertyWatchOfType&) = delete;
    PropertyWatchOfType& operator=(PropertyWatchOfType&&) = default;
    ~PropertyWatchOfType() = default;
    PropertyWatchOfType(const PropertyIndex& watchIndex, Callback& callback,
                        Filters* filterOps = nullptr) :
        PropertyWatch<DBusInterfaceType>(watchIndex, &callback),
        filterOps(filterOps)
    {
    }
    PropertyWatchOfType(const PropertyIndex& watchIndex,
                        Filters* filterOps = nullptr) :
        PropertyWatch<DBusInterfaceType>(watchIndex, nullptr),
        filterOps(filterOps)
    {
    }

    /** @brief PropertyMatch implementation for PropertyWatchOfType.
     *
     *  @param[in] busName - The busname hosting the interface to query.
     *  @param[in] path - The path of the interface to query.
     *  @param[in] interface - The interface to query.
     */
    void updateProperties(const std::string& busName, const std::string& path,
                          const std::string& interface) override;

    /** @brief PropertyMatch implementation for PropertyWatchOfType.
     *
     *  @param[in] msg - The org.freedesktop.DBus.PropertiesChanged
     *               message.
     *  @param[in] path - The path associated with the message.
     *  @param[in] interface - The interface associated with the message.
     */
    void propertiesChanged(sdbusplus::message::message& msg,
                           const std::string& path,
                           const std::string& interface) override;

    /** @brief DBus agnostic implementation of interfacesAdded.
     *
     *  @param[in] path - The path of the properties that changed.
     *  @param[in] interface - The interface of the properties that
     *                  changed.
     *  @param[in] properites - The properties that changed.
     */
    void propertiesChanged(const std::string& path,
                           const std::string& interface,
                           const PropertiesChanged<T>& properties);

    /** @brief PropertyMatch implementation for PropertyWatchOfType.
     *
     *  @param[in] msg - The org.freedesktop.DBus.PropertiesChanged
     *               message.
     */
    void interfacesAdded(sdbusplus::message::message& msg) override;

    /** @brief DBus agnostic implementation of interfacesAdded.
     *
     *  @param[in] path - The path of the added interfaces.
     *  @param[in] interfaces - The added interfaces.
     */
    void interfacesAdded(const std::string& path,
                         const InterfacesAdded<T>& interfaces);

  private:
    /** @brief Optional filter operations to perform on property changes. */
    Filters* const filterOps;
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
