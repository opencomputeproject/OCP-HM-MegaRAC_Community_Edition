#pragma once

#include "types.hpp"
#include "utils.hpp"

#include <memory>
#include <sdbusplus/bus.hpp>
#include <utility>

namespace phosphor
{
namespace inventory
{
namespace manager
{

class Manager;

/** @brief make_action
 *
 *  Adapt an action function object.
 *
 *  @param[in] action - The action being adapted.
 *  @returns - The adapted action.
 *
 *  @tparam T - The type of the action being adapted.
 */
template <typename T>
auto make_action(T&& action)
{
    return Action(std::forward<T>(action));
}

/** @brief make_filter
 *
 *  Adapt a filter function object.
 *
 *  @param[in] filter - The filter being adapted.
 *  @returns - The adapted filter.
 *
 *  @tparam T - The type of the filter being adapted.
 */
template <typename T>
auto make_filter(T&& filter)
{
    return Filter(std::forward<T>(filter));
}

/** @brief make_path_condition
 *
 *  Adapt a path_condition function object.
 *
 *  @param[in] filter - The functor being adapted.
 *  @returns - The adapted functor.
 *
 *  @tparam T - The type of the functor being adapted.
 */
template <typename T>
auto make_path_condition(T&& condition)
{
    return PathCondition(std::forward<T>(condition));
}

/** @brief make_get_property
 *
 *  Adapt a get_property function object.
 *
 *  @param[in] method - The functor being adapted.
 *  @returns - The adapted functor.
 *
 *  @tparam T - The return type of the function object.
 *  @tparam U - The type of the functor being adapted.
 */
template <typename T, typename U>
auto make_get_property(U&& method)
{
    return GetProperty<T>(std::forward<U>(method));
}

template <typename T, typename... Args>
auto callArrayWithStatus(T&& container, Args&&... args)
{
    for (auto f : container)
    {
        if (!f(std::forward<Args>(args)...))
        {
            return false;
        }
    }
    return true;
}

namespace functor
{

/** @brief Destroy objects action.  */
inline auto destroyObjects(std::vector<const char*>&& paths,
                           std::vector<PathCondition>&& conditions)
{
    return [=](auto& b, auto& m) {
        for (const auto& p : paths)
        {
            if (callArrayWithStatus(conditions, p, b, m))
            {
                m.destroyObjects({p});
            }
        }
    };
}

/** @brief Create objects action.  */
inline auto
    createObjects(std::map<sdbusplus::message::object_path, Object>&& objs)
{
    return [=](auto&, auto& m) { m.createObjects(objs); };
}

/** @brief Set a property action.
 *
 *  Invoke the requested method with a reference to the requested
 *  sdbusplus server binding interface as a parameter.
 *
 *  @tparam T - The sdbusplus server binding interface type.
 *  @tparam U - The type of the sdbusplus server binding member
 *      function that sets the property.
 *  @tparam V - The property value type.
 *
 *  @param[in] paths - The DBus paths on which the property should
 *      be set.
 *  @param[in] iface - The DBus interface hosting the property.
 *  @param[in] member - Pointer to sdbusplus server binding member.
 *  @param[in] value - The value the property should be set to.
 *
 *  @returns - A function object that sets the requested property
 *      to the requested value.
 */
template <typename T, typename U, typename V>
auto setProperty(std::vector<const char*>&& paths,
                 std::vector<PathCondition>&& conditions, const char* iface,
                 U&& member, V&& value)
{
    // The manager is the only parameter passed to actions.
    // Bind the path, interface, interface member function pointer,
    // and value to a lambda.  When it is called, forward the
    // path, interface and value on to the manager member function.
    return [paths, conditions = conditions, iface, member,
            value = std::forward<V>(value)](auto& b, auto& m) {
        for (auto p : paths)
        {
            if (callArrayWithStatus(conditions, p, b, m))
            {
                m.template invokeMethod<T>(p, iface, member, value);
            }
        }
    };
}

/** @brief Get a property.
 *
 *  Invoke the requested method with a reference to the requested
 *  sdbusplus server binding interface as a parameter.
 *
 *  @tparam T - The sdbusplus server binding interface type.
 *  @tparam U - The type of the sdbusplus server binding member
 *      function that sets the property.
 *
 *  @param[in] path - The DBus path to get the property from.
 *  @param[in] iface - The DBus interface hosting the property.
 *  @param[in] member - Pointer to sdbusplus server binding member.
 *  @param[in] prop - The property name to get the value from.
 *
 *  @returns - A function object that gets the requested property.
 */
template <typename T, typename U>
inline auto getProperty(const char* path, const char* iface, U&& member,
                        const char* prop)
{
    return [path, iface, member, prop](auto& mgr) {
        return mgr.template invokeMethod<T>(path, iface, member, prop);
    };
}

/** @struct PropertyChangedCondition
 *  @brief Match filter functor that tests a property value.
 *
 *  @tparam T - The type of the property being tested.
 *  @tparam U - The type of the condition checking functor.
 */
template <typename T, typename U>
struct PropertyChangedCondition
{
    PropertyChangedCondition() = delete;
    ~PropertyChangedCondition() = default;
    PropertyChangedCondition(const PropertyChangedCondition&) = default;
    PropertyChangedCondition&
        operator=(const PropertyChangedCondition&) = default;
    PropertyChangedCondition(PropertyChangedCondition&&) = default;
    PropertyChangedCondition& operator=(PropertyChangedCondition&&) = default;
    PropertyChangedCondition(const char* iface, const char* property,
                             U&& condition) :
        _iface(iface),
        _property(property), _condition(std::forward<U>(condition))
    {
    }

    /** @brief Test a property value.
     *
     * Extract the property from the PropertiesChanged
     * message and run the condition test.
     */
    bool operator()(sdbusplus::bus::bus&, sdbusplus::message::message& msg,
                    Manager&) const
    {
        std::map<std::string, std::variant<T>> properties;
        const char* iface = nullptr;

        msg.read(iface);
        if (!iface || strcmp(iface, _iface))
        {
            return false;
        }

        msg.read(properties);
        auto it = properties.find(_property);
        if (it == properties.cend())
        {
            return false;
        }

        return _condition(std::forward<T>(std::get<T>(it->second)));
    }

  private:
    const char* _iface;
    const char* _property;
    U _condition;
};

/** @struct PropertyConditionBase
 *  @brief Match filter functor that tests a property value.
 *
 *  Base class for PropertyCondition - factored out code that
 *  doesn't need to be templated.
 */
struct PropertyConditionBase
{
    PropertyConditionBase() = delete;
    virtual ~PropertyConditionBase() = default;
    PropertyConditionBase(const PropertyConditionBase&) = default;
    PropertyConditionBase& operator=(const PropertyConditionBase&) = default;
    PropertyConditionBase(PropertyConditionBase&&) = default;
    PropertyConditionBase& operator=(PropertyConditionBase&&) = default;

    /** @brief Constructor
     *
     *  The service argument can be nullptr.  If something
     *  else is provided the function will call the the
     *  service directly.  If omitted, the function will
     *  look up the service in the ObjectMapper.
     *
     *  @param path - The path of the object containing
     *     the property to be tested.
     *  @param iface - The interface hosting the property
     *     to be tested.
     *  @param property - The property to be tested.
     *  @param service - The DBus service hosting the object.
     */
    PropertyConditionBase(const char* path, const char* iface,
                          const char* property, const char* service) :
        _path(path ? path : std::string()),
        _iface(iface), _property(property), _service(service)
    {
    }

    /** @brief Forward comparison to type specific implementation. */
    virtual bool eval(sdbusplus::message::message&) const = 0;

    /** @brief Forward comparison to type specific implementation. */
    virtual bool eval(Manager&) const = 0;

    /** @brief Test a property value.
     *
     * Make a DBus call and test the value of any property.
     */
    bool operator()(sdbusplus::bus::bus&, sdbusplus::message::message&,
                    Manager&) const;

    /** @brief Test a property value.
     *
     * Make a DBus call and test the value of any property.
     */
    bool operator()(const std::string&, sdbusplus::bus::bus&, Manager&) const;

  private:
    std::string _path;
    std::string _iface;
    std::string _property;
    const char* _service;
};

/** @struct PropertyCondition
 *  @brief Match filter functor that tests a property value.
 *
 *  @tparam T - The type of the property being tested.
 *  @tparam U - The type of the condition checking functor.
 *  @tparam V - The getProperty functor return type.
 */
template <typename T, typename U, typename V>
struct PropertyCondition final : public PropertyConditionBase
{
    PropertyCondition() = delete;
    ~PropertyCondition() = default;
    PropertyCondition(const PropertyCondition&) = default;
    PropertyCondition& operator=(const PropertyCondition&) = default;
    PropertyCondition(PropertyCondition&&) = default;
    PropertyCondition& operator=(PropertyCondition&&) = default;

    /** @brief Constructor
     *
     *  The service & getProperty arguments can be nullptrs.
     *  If something else is provided the function will call the the
     *  service directly.  If omitted, the function will
     *  look up the service in the ObjectMapper.
     *  The getProperty function will be called to retrieve a property
     *  value when given and the property is hosted by inventory manager.
     *  When not given, the condition will default to return that the
     *  condition failed and will not be executed.
     *
     *  @param path - The path of the object containing
     *     the property to be tested.
     *  @param iface - The interface hosting the property
     *     to be tested.
     *  @param property - The property to be tested.
     *  @param condition - The test to run on the property.
     *  @param service - The DBus service hosting the object.
     *  @param getProperty - The function to get a property value
     *     for the condition.
     */
    PropertyCondition(const char* path, const char* iface, const char* property,
                      U&& condition, const char* service,
                      GetProperty<V>&& getProperty = nullptr) :
        PropertyConditionBase(path, iface, property, service),
        _condition(std::forward<decltype(condition)>(condition)),
        _getProperty(getProperty)
    {
    }

    /** @brief Test a property value.
     *
     * Make a DBus call and test the value of any property.
     */
    bool eval(sdbusplus::message::message& msg) const override
    {
        std::variant<T> value;
        msg.read(value);
        return _condition(std::forward<T>(std::get<T>(value)));
    }

    /** @brief Retrieve a property value from inventory and test it.
     *
     *  Get a property from the inventory manager and test the value.
     *  Default to fail the test where no function is given to get the
     *  property from the inventory manager.
     */
    bool eval(Manager& mgr) const override
    {
        if (_getProperty)
        {
            auto variant = _getProperty(mgr);
            auto value = std::get<T>(variant);
            return _condition(std::forward<T>(value));
        }
        return false;
    }

  private:
    U _condition;
    GetProperty<V> _getProperty;
};

/** @brief Implicit type deduction for constructing PropertyChangedCondition. */
template <typename T>
auto propertyChangedTo(const char* iface, const char* property, T&& val)
{
    auto condition = [val = std::forward<T>(val)](T&& arg) {
        return arg == val;
    };
    using U = decltype(condition);
    return PropertyChangedCondition<T, U>(iface, property,
                                          std::move(condition));
}

/** @brief Implicit type deduction for constructing PropertyCondition.  */
template <typename T, typename V = InterfaceVariantType>
auto propertyIs(const char* path, const char* iface, const char* property,
                T&& val, const char* service = nullptr,
                GetProperty<V>&& getProperty = nullptr)
{
    auto condition = [val = std::forward<T>(val)](T&& arg) {
        return arg == val;
    };
    using U = decltype(condition);
    return PropertyCondition<T, U, V>(path, iface, property,
                                      std::move(condition), service,
                                      std::move(getProperty));
}
} // namespace functor
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
