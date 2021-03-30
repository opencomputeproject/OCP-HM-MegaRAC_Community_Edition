#pragma once
#include "data_types.hpp"
#include "sdbusplus/bus/match.hpp"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/** @class CallMethodAndRead
 *  @brief GMock template member forwarding helper.
 *
 *  The code under test calls callMethodAndRead, which is a templated,
 *  free function.  Enable this under GMock by forwarding calls to it
 *  to functions that can be mocked.
 *
 *  @tparam DBusInterfaceType - The mock object type.
 *  @tparam Ret - The return type of the method being called.
 *  @tparam Args - The argument types of the method being called.
 *
 *  Specialize to implement new forwards.
 */
template <typename DBusInterfaceType, typename Ret, typename... Args>
struct CallMethodAndRead
{
    static Ret op(DBusInterfaceType& dbus, const std::string& busName,
                  const std::string& path, const std::string& interface,
                  const std::string& method, Args&&... args)
    {
        static_assert(true, "Missing CallMethodAndRead definition.");
        return Ret();
    }
};

/** @brief CallMethodAndRead specialization for
 *     xyz.openbmc_project.ObjectMapper.GetObject. */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, GetObject, const MapperPath&,
                         const std::vector<std::string>&>
{
    static GetObject op(DBusInterfaceType& dbus, const std::string& busName,
                        const std::string& path, const std::string& interface,
                        const std::string& method, const MapperPath& objectPath,
                        const std::vector<std::string>& interfaces)
    {
        return dbus.mapperGetObject(busName, path, interface, method,
                                    objectPath, interfaces);
    }
};

/** @brief CallMethodAndRead specialization for
 *     org.freedesktop.DBus.Properties.GetAll(uint64_t). */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, PropertiesChanged<uint64_t>,
                         const std::string&>
{
    static PropertiesChanged<uint64_t>
        op(DBusInterfaceType& dbus, const std::string& busName,
           const std::string& path, const std::string& interface,
           const std::string& method, const std::string& propertiesInterface)
    {
        return dbus.getPropertiesU64(busName, path, interface, method,
                                     propertiesInterface);
    }
};

/** @brief CallMethodAndRead specialization for
 *     org.freedesktop.DBus.Properties.GetAll(uint32_t). */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, PropertiesChanged<uint32_t>,
                         const std::string&>
{
    static PropertiesChanged<uint32_t>
        op(DBusInterfaceType& dbus, const std::string& busName,
           const std::string& path, const std::string& interface,
           const std::string& method, const std::string& propertiesInterface)
    {
        return dbus.getPropertiesU32(busName, path, interface, method,
                                     propertiesInterface);
    }
};

/** @brief CallMethodAndRead specialization for
 *     org.freedesktop.DBus.Properties.GetAll(uint16_t). */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, PropertiesChanged<uint16_t>,
                         const std::string&>
{
    static PropertiesChanged<uint16_t>
        op(DBusInterfaceType& dbus, const std::string& busName,
           const std::string& path, const std::string& interface,
           const std::string& method, const std::string& propertiesInterface)
    {
        return dbus.getPropertiesU16(busName, path, interface, method,
                                     propertiesInterface);
    }
};

/** @brief CallMethodAndRead specialization for
 *     org.freedesktop.DBus.Properties.GetAll(uint8_t). */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, PropertiesChanged<uint8_t>,
                         const std::string&>
{
    static PropertiesChanged<uint8_t>
        op(DBusInterfaceType& dbus, const std::string& busName,
           const std::string& path, const std::string& interface,
           const std::string& method, const std::string& propertiesInterface)
    {
        return dbus.getPropertiesU8(busName, path, interface, method,
                                    propertiesInterface);
    }
};

/** @brief CallMethodAndRead specialization for
 *     org.freedesktop.DBus.Properties.GetAll(int64_t). */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, PropertiesChanged<int64_t>,
                         const std::string&>
{
    static PropertiesChanged<int64_t>
        op(DBusInterfaceType& dbus, const std::string& busName,
           const std::string& path, const std::string& interface,
           const std::string& method, const std::string& propertiesInterface)
    {
        return dbus.getPropertiesU64(busName, path, interface, method,
                                     propertiesInterface);
    }
};

/** @brief CallMethodAndRead specialization for
 *     org.freedesktop.DBus.Properties.GetAll(int32_t). */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, PropertiesChanged<int32_t>,
                         const std::string&>
{
    static PropertiesChanged<int32_t>
        op(DBusInterfaceType& dbus, const std::string& busName,
           const std::string& path, const std::string& interface,
           const std::string& method, const std::string& propertiesInterface)
    {
        return dbus.getPropertiesU32(busName, path, interface, method,
                                     propertiesInterface);
    }
};

/** @brief CallMethodAndRead specialization for
 *     org.freedesktop.DBus.Properties.GetAll(int16_t). */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, PropertiesChanged<int16_t>,
                         const std::string&>
{
    static PropertiesChanged<int16_t>
        op(DBusInterfaceType& dbus, const std::string& busName,
           const std::string& path, const std::string& interface,
           const std::string& method, const std::string& propertiesInterface)
    {
        return dbus.getPropertiesU16(busName, path, interface, method,
                                     propertiesInterface);
    }
};

/** @brief CallMethodAndRead specialization for
 *     org.freedesktop.DBus.Properties.GetAll(int8_t). */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, PropertiesChanged<int8_t>,
                         const std::string&>
{
    static PropertiesChanged<int8_t>
        op(DBusInterfaceType& dbus, const std::string& busName,
           const std::string& path, const std::string& interface,
           const std::string& method, const std::string& propertiesInterface)
    {
        return dbus.getPropertiesU8(busName, path, interface, method,
                                    propertiesInterface);
    }
};

/** @brief CallMethodAndRead specialization for
 *     org.freedesktop.DBus.Properties.GetAll(std::string). */
template <typename DBusInterfaceType>
struct CallMethodAndRead<DBusInterfaceType, PropertiesChanged<std::string>,
                         const std::string&>
{
    static PropertiesChanged<std::string>
        op(DBusInterfaceType& dbus, const std::string& busName,
           const std::string& path, const std::string& interface,
           const std::string& method, const std::string& propertiesInterface)
    {
        return dbus.getPropertiesString(busName, path, interface, method,
                                        propertiesInterface);
    }
};

/** @class MockDBusInterface
 *  @brief DBus access delegate implementation for the property watch test
 *  suite.
 */
struct MockDBusInterface
{
    MOCK_METHOD6(mapperGetObject,
                 GetObject(const std::string&, const std::string&,
                           const std::string&, const std::string&,
                           const MapperPath&, const std::vector<std::string>&));

    MOCK_METHOD5(getPropertiesU64,
                 PropertiesChanged<uint64_t>(
                     const std::string&, const std::string&, const std::string&,
                     const std::string&, const std::string&));

    MOCK_METHOD5(getPropertiesU32,
                 PropertiesChanged<uint32_t>(
                     const std::string&, const std::string&, const std::string&,
                     const std::string&, const std::string&));

    MOCK_METHOD5(getPropertiesU16,
                 PropertiesChanged<uint16_t>(
                     const std::string&, const std::string&, const std::string&,
                     const std::string&, const std::string&));

    MOCK_METHOD5(getPropertiesU8,
                 PropertiesChanged<uint8_t>(
                     const std::string&, const std::string&, const std::string&,
                     const std::string&, const std::string&));

    MOCK_METHOD5(getPropertiesS64,
                 PropertiesChanged<int64_t>(
                     const std::string&, const std::string&, const std::string&,
                     const std::string&, const std::string&));

    MOCK_METHOD5(getPropertiesS32,
                 PropertiesChanged<int32_t>(
                     const std::string&, const std::string&, const std::string&,
                     const std::string&, const std::string&));

    MOCK_METHOD5(getPropertiesS16,
                 PropertiesChanged<int16_t>(
                     const std::string&, const std::string&, const std::string&,
                     const std::string&, const std::string&));

    MOCK_METHOD5(getPropertiesS8,
                 PropertiesChanged<int8_t>(
                     const std::string&, const std::string&, const std::string&,
                     const std::string&, const std::string&));

    MOCK_METHOD5(getPropertiesString,
                 PropertiesChanged<std::string>(
                     const std::string&, const std::string&, const std::string&,
                     const std::string&, const std::string&));

    MOCK_METHOD2(fwdAddMatch,
                 void(const std::string&,
                      const sdbusplus::bus::match::match::callback_t&));

    static MockDBusInterface* ptr;
    static MockDBusInterface& instance()
    {
        return *ptr;
    }
    static void instance(MockDBusInterface& p)
    {
        ptr = &p;
    }

    /** @brief GMock member template/free function forward. */
    template <typename Ret, typename... Args>
    static auto callMethodAndRead(const std::string& busName,
                                  const std::string& path,
                                  const std::string& interface,
                                  const std::string& method, Args&&... args)
    {
        return CallMethodAndRead<MockDBusInterface, Ret, Args...>::op(
            instance(), busName, path, interface, method,
            std::forward<Args>(args)...);
    }

    /** @brief GMock free function forward. */
    static auto
        addMatch(const std::string& match,
                 const sdbusplus::bus::match::match::callback_t& callback)
    {
        instance().fwdAddMatch(match, callback);
    }
};

/** @class Expect
 *  @brief Enable use of EXPECT_CALL from a C++ template.
 */
template <typename T>
struct Expect
{
};

template <>
struct Expect<uint64_t>
{
    template <typename MockObjType>
    static auto& getProperties(MockObjType&& mockObj, const std::string& path,
                               const std::string& interface)
    {
        return EXPECT_CALL(std::forward<MockObjType>(mockObj),
                           getPropertiesU64(::testing::_, path,
                                            "org.freedesktop.DBus.Properties",
                                            "GetAll", interface));
    }
};

template <>
struct Expect<uint32_t>
{
    template <typename MockObjType>
    static auto& getProperties(MockObjType&& mockObj, const std::string& path,
                               const std::string& interface)
    {
        return EXPECT_CALL(std::forward<MockObjType>(mockObj),
                           getPropertiesU32(::testing::_, path,
                                            "org.freedesktop.DBus.Properties",
                                            "GetAll", interface));
    }
};

template <>
struct Expect<uint16_t>
{
    template <typename MockObjType>
    static auto& getProperties(MockObjType&& mockObj, const std::string& path,
                               const std::string& interface)
    {
        return EXPECT_CALL(std::forward<MockObjType>(mockObj),
                           getPropertiesU16(::testing::_, path,
                                            "org.freedesktop.DBus.Properties",
                                            "GetAll", interface));
    }
};

template <>
struct Expect<uint8_t>
{
    template <typename MockObjType>
    static auto& getProperties(MockObjType&& mockObj, const std::string& path,
                               const std::string& interface)
    {
        return EXPECT_CALL(std::forward<MockObjType>(mockObj),
                           getPropertiesU8(::testing::_, path,
                                           "org.freedesktop.DBus.Properties",
                                           "GetAll", interface));
    }
};

template <>
struct Expect<int64_t>
{
    template <typename MockObjType>
    static auto& getProperties(MockObjType&& mockObj, const std::string& path,
                               const std::string& interface)
    {
        return EXPECT_CALL(std::forward<MockObjType>(mockObj),
                           getPropertiesS64(::testing::_, path,
                                            "org.freedesktop.DBus.Properties",
                                            "GetAll", interface));
    }
};

template <>
struct Expect<int32_t>
{
    template <typename MockObjType>
    static auto& getProperties(MockObjType&& mockObj, const std::string& path,
                               const std::string& interface)
    {
        return EXPECT_CALL(std::forward<MockObjType>(mockObj),
                           getPropertiesS32(::testing::_, path,
                                            "org.freedesktop.DBus.Properties",
                                            "GetAll", interface));
    }
};

template <>
struct Expect<int16_t>
{
    template <typename MockObjType>
    static auto& getProperties(MockObjType&& mockObj, const std::string& path,
                               const std::string& interface)
    {
        return EXPECT_CALL(std::forward<MockObjType>(mockObj),
                           getPropertiesS16(::testing::_, path,
                                            "org.freedesktop.DBus.Properties",
                                            "GetAll", interface));
    }
};

template <>
struct Expect<int8_t>
{
    template <typename MockObjType>
    static auto& getProperties(MockObjType&& mockObj, const std::string& path,
                               const std::string& interface)
    {
        return EXPECT_CALL(std::forward<MockObjType>(mockObj),
                           getPropertiesS8(::testing::_, path,
                                           "org.freedesktop.DBus.Properties",
                                           "GetAll", interface));
    }
};

template <>
struct Expect<std::string>
{
    template <typename MockObjType>
    static auto& getProperties(MockObjType&& mockObj, const std::string& path,
                               const std::string& interface)
    {
        return EXPECT_CALL(
            std::forward<MockObjType>(mockObj),
            getPropertiesString(::testing::_, path,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                interface));
    }
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
