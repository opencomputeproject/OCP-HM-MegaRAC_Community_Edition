#include "propertywatchtest.hpp"

#include "propertywatchimpl.hpp"

#include <array>
#include <functional>

using namespace std::string_literals;
using namespace phosphor::dbus::monitoring;

const std::array<std::string, 4> paths = {
    "/xyz/openbmc_project/testing/inst1"s,
    "/xyz/openbmc_project/testing/inst2"s,
    "/xyz/openbmc_project/testing/inst3"s,
    "/xyz/openbmc_project/testing/inst4"s,
};

const std::array<std::string, 2> interfaces = {
    "xyz.openbmc_project.Iface1"s,
    "xyz.openbmc_project.Iface2"s,
};

const std::array<std::string, 2> properties = {
    "Value1"s,
    "Value2"s,
};

const std::string meta;

std::array<std::tuple<any_ns::any, any_ns::any>, 8> storage = {};

const PropertyIndex watchIndex = {
    {
        {PropertyIndex::key_type{paths[0], interfaces[0], properties[0]},
         PropertyIndex::mapped_type{meta, meta, storage[0]}},
        {PropertyIndex::key_type{paths[0], interfaces[1], properties[1]},
         PropertyIndex::mapped_type{meta, meta, storage[1]}},
        {PropertyIndex::key_type{paths[1], interfaces[0], properties[0]},
         PropertyIndex::mapped_type{meta, meta, storage[2]}},
        {PropertyIndex::key_type{paths[1], interfaces[1], properties[1]},
         PropertyIndex::mapped_type{meta, meta, storage[3]}},
        {PropertyIndex::key_type{paths[2], interfaces[0], properties[0]},
         PropertyIndex::mapped_type{meta, meta, storage[4]}},
        {PropertyIndex::key_type{paths[2], interfaces[1], properties[1]},
         PropertyIndex::mapped_type{meta, meta, storage[5]}},
        {PropertyIndex::key_type{paths[3], interfaces[0], properties[0]},
         PropertyIndex::mapped_type{meta, meta, storage[6]}},
        {PropertyIndex::key_type{paths[3], interfaces[1], properties[1]},
         PropertyIndex::mapped_type{meta, meta, storage[7]}},
    },
};

template <typename T>
struct Values
{
};
template <>
struct Values<uint8_t>
{
    static auto& get(size_t i)
    {
        static const std::array<uint8_t, 8> values = {
            {0, 1, 2, 3, 4, 5, 6, 7},
        };
        return values[i];
    }
};

template <>
struct Values<uint16_t>
{
    static auto& get(size_t i)
    {
        static const std::array<uint16_t, 8> values = {
            {88, 77, 66, 55, 44, 33, 22, 11},
        };
        return values[i];
    }
};

template <>
struct Values<uint32_t>
{
    static auto& get(size_t i)
    {
        static const std::array<uint32_t, 8> values = {
            {0xffffffff, 1, 3, 0, 5, 7, 9, 0xffffffff},
        };
        return values[i];
    }
};

template <>
struct Values<uint64_t>
{
    static auto& get(size_t i)
    {
        static const std::array<uint64_t, 8> values = {
            {0xffffffffffffffff, 3, 7, 12234, 0, 3, 9, 0xffffffff},
        };
        return values[i];
    }
};

template <>
struct Values<std::string>
{
    static auto& get(size_t i)
    {
        static const std::array<std::string, 8> values = {
            {""s, "foo"s, "bar"s, "baz"s, "hello"s, "string", "\x2\x3", "\\"},
        };
        return values[i];
    }
};

template <typename T>
void nonFilteredCheck(const any_ns::any& value, const size_t ndx)
{
    ASSERT_EQ(value.empty(), false);
    ASSERT_EQ(any_ns::any_cast<T>(value), Values<T>::get(ndx));
}

template <typename T>
struct FilteredValues
{
};

template <>
struct FilteredValues<uint8_t>
{
    static auto& opFilters()
    {
        static std::unique_ptr<OperandFilters<uint8_t>> filters =
            std::make_unique<OperandFilters<uint8_t>>(
                std::vector<std::function<bool(uint8_t)>>{
                    [](const auto& value) { return value < 4; }});
        return filters;
    }
    static auto& expected(size_t i)
    {
        static const std::array<any_ns::any, 8> values = {
            {any_ns::any(uint8_t(0)), any_ns::any(uint8_t(1)),
             any_ns::any(uint8_t(2)), any_ns::any(uint8_t(3)), any_ns::any(),
             any_ns::any(), any_ns::any(), any_ns::any()}};
        return values[i];
    }
};

template <>
struct FilteredValues<uint16_t>
{
    static auto& opFilters()
    {
        static std::unique_ptr<OperandFilters<uint16_t>> filters =
            std::make_unique<OperandFilters<uint16_t>>(
                std::vector<std::function<bool(uint16_t)>>{
                    [](const auto& value) { return value > 44; },
                    [](const auto& value) { return value != 88; }});
        return filters;
    }
    static auto& expected(size_t i)
    {
        static const std::array<any_ns::any, 8> values = {
            {any_ns::any(), any_ns::any(uint16_t(77)),
             any_ns::any(uint16_t(66)), any_ns::any(uint16_t(55)),
             any_ns::any(), any_ns::any(), any_ns::any(), any_ns::any()}};
        return values[i];
    }
};

template <>
struct FilteredValues<uint32_t>
{
    static auto& opFilters()
    {
        static std::unique_ptr<OperandFilters<uint32_t>> filters =
            std::make_unique<OperandFilters<uint32_t>>(
                std::vector<std::function<bool(uint32_t)>>{
                    [](const auto& value) { return value != 0xffffffff; },
                    [](const auto& value) { return value != 0; }});
        return filters;
    }
    static auto& expected(size_t i)
    {
        static const std::array<any_ns::any, 8> values = {
            {any_ns::any(), any_ns::any(uint32_t(1)), any_ns::any(uint32_t(3)),
             any_ns::any(), any_ns::any(uint32_t(5)), any_ns::any(uint32_t(7)),
             any_ns::any(uint32_t(9)), any_ns::any()}};
        return values[i];
    }
};

template <>
struct FilteredValues<uint64_t>
{
    static auto& opFilters()
    {
        static std::unique_ptr<OperandFilters<uint64_t>> filters =
            std::make_unique<OperandFilters<uint64_t>>(
                std::vector<std::function<bool(uint64_t)>>{
                    [](const auto& value) { return (value % 3) != 0; }});
        return filters;
    }
    static auto& expected(size_t i)
    {
        static const std::array<any_ns::any, 8> values = {
            {any_ns::any(), any_ns::any(), any_ns::any(uint64_t(7)),
             any_ns::any(), any_ns::any(), any_ns::any(), any_ns::any(),
             any_ns::any()}};
        return values[i];
    }
};

template <>
struct FilteredValues<std::string>
{
    static auto& opFilters()
    {
        static std::unique_ptr<OperandFilters<std::string>> filters =
            std::make_unique<OperandFilters<std::string>>(
                std::vector<std::function<bool(std::string)>>{
                    [](const auto& value) { return value != ""s; },
                    [](const auto& value) { return value != "string"s; }});
        return filters;
    }
    static auto& expected(size_t i)
    {
        static const std::array<any_ns::any, 8> values = {
            {any_ns::any(), any_ns::any("foo"s), any_ns::any("bar"s),
             any_ns::any("baz"s), any_ns::any("hello"s), any_ns::any(),
             any_ns::any("\x2\x3"s), any_ns::any("\\"s)}};
        return values[i];
    }
};

template <typename T>
void filteredCheck(const any_ns::any& value, const size_t ndx)
{
    ASSERT_EQ(value.empty(), FilteredValues<T>::expected(ndx).empty());
    if (!value.empty())
    {
        ASSERT_EQ(any_ns::any_cast<T>(value),
                  any_ns::any_cast<T>(FilteredValues<T>::expected(ndx)));
    }
}

template <typename T>
void testStart(
    std::function<void(const any_ns::any&, const size_t)>&& checkState,
    OperandFilters<T>* opFilters = nullptr)
{
    using ::testing::_;
    using ::testing::Return;

    MockDBusInterface dbus;
    MockDBusInterface::instance(dbus);

    const std::vector<std::string> expectedMapperInterfaces;
    PropertyWatchOfType<T, MockDBusInterface> watch(watchIndex, opFilters);

    auto ndx = static_cast<size_t>(0);
    for (const auto& o : convert(watchIndex))
    {
        const auto& path = o.first.get();
        const auto& interfaces = o.second;
        std::vector<std::string> mapperResponse;
        std::transform(interfaces.begin(), interfaces.end(),
                       std::back_inserter(mapperResponse),
                       // *INDENT-OFF*
                       [](const auto& item) { return item.first; });
        // *INDENT-ON*
        EXPECT_CALL(dbus, mapperGetObject(MAPPER_BUSNAME, MAPPER_PATH,
                                          MAPPER_INTERFACE, "GetObject", path,
                                          expectedMapperInterfaces))
            .WillOnce(Return(GetObject({{"", mapperResponse}})));
        EXPECT_CALL(
            dbus, fwdAddMatch(
                      sdbusplus::bus::match::rules::interfacesAdded(path), _));
        for (const auto& i : interfaces)
        {
            const auto& interface = i.first.get();
            const auto& properties = i.second;
            EXPECT_CALL(
                dbus,
                fwdAddMatch(sdbusplus::bus::match::rules::propertiesChanged(
                                path, interface),
                            _));

            PropertiesChanged<T> serviceResponse;
            for (const auto& p : properties)
            {
                serviceResponse[p] = Values<T>::get(ndx);
                ++ndx;
            }
            Expect<T>::getProperties(dbus, path, interface)
                .WillOnce(Return(serviceResponse));
        }
    }

    watch.start();

    ndx = 0;
    for (auto s : storage)
    {
        checkState(std::get<valueIndex>(s), ndx);
        ++ndx;
    }

    // Make sure start logic only runs the first time.
    watch.start();
}

TEST(PropertyWatchTest, TestStart)
{
    testStart<uint8_t>(nonFilteredCheck<uint8_t>);
    testStart<uint16_t>(nonFilteredCheck<uint16_t>);
    testStart<uint32_t>(nonFilteredCheck<uint32_t>);
    testStart<uint64_t>(nonFilteredCheck<uint64_t>);
    testStart<std::string>(nonFilteredCheck<std::string>);
}

TEST(PropertyWatchTest, TestFilters)
{
    testStart<uint8_t>(filteredCheck<uint8_t>,
                       FilteredValues<uint8_t>::opFilters().get());
    testStart<uint16_t>(filteredCheck<uint16_t>,
                        FilteredValues<uint16_t>::opFilters().get());
    testStart<uint32_t>(filteredCheck<uint32_t>,
                        FilteredValues<uint32_t>::opFilters().get());
    testStart<uint64_t>(filteredCheck<uint64_t>,
                        FilteredValues<uint64_t>::opFilters().get());
    testStart<std::string>(filteredCheck<std::string>,
                           FilteredValues<std::string>::opFilters().get());
}

MockDBusInterface* MockDBusInterface::ptr = nullptr;
