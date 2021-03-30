#pragma once

#include "tupleref.hpp"

#include <experimental/any>
#include <sdbusplus/message.hpp>
#include <string>

namespace any_ns = std::experimental;

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

// PropertyIndex::key_type fields
constexpr auto pathIndex = 0;
constexpr auto interfaceIndex = 1;
constexpr auto propertyIndex = 2;

// PropertyIndex::mapped_type fields
constexpr auto pathMetaIndex = 0;
constexpr auto propertyMetaIndex = 1;
constexpr auto storageIndex = 2;

// ConfigPropertyStorage fields
constexpr auto valueIndex = 0;
constexpr auto resultIndex = 1;

enum class Context
{
    START,
    SIGNAL,
};

/** @brief A map with references as keys. */
template <typename Key, typename Value>
using RefKeyMap = std::map<std::reference_wrapper<Key>, Value, std::less<Key>>;

/** @brief A map with a tuple of references as keys. */
template <typename Value, typename... Keys>
using TupleRefMap = std::map<TupleOfRefs<Keys...>, Value, TupleOfRefsLess>;

/** @brief A vector of references. */
template <typename T>
using RefVector = std::vector<std::reference_wrapper<T>>;

/** @brief
 *
 *  The mapper has a defect such that it provides strings
 *  rather than object paths.  Use an alias for easy refactoring
 *  when the mapper is fixed.
 */
using MapperPath = std::string;

/** @brief ObjectManager.InterfacesAdded signal signature alias. */
template <typename T>
using InterfacesAdded =
    std::map<std::string, std::map<std::string, std::variant<T>>>;
using Value = std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                           int64_t, uint64_t, std::string>;

/** @brief ObjectManager.InterfacesAdded signal signature alias. */
using Interface = std::string;
using Property = std::string;
using PathInterfacesAdded = std::map<Interface, std::map<Property, Value>>;

/** @brief ObjectMapper.GetObject response signature alias. */
using GetObject = std::map<MapperPath, std::vector<std::string>>;

/** @brief Properties.GetAll response signature alias. */
template <typename T>
using PropertiesChanged = std::map<std::string, std::variant<T>>;

/** @brief Lookup index for properties . */
// *INDENT-OFF*
using PropertyIndex =
    TupleRefMap<TupleOfRefs<const std::string, const std::string,
                            std::tuple<any_ns::any, any_ns::any>>,
                const std::string, const std::string, const std::string>;
// *INDENT-ON*

/** @brief Convert some C++ types to others.
 *
 *  Remove type decorators to reduce template specializations.
 *
 *  1. Remove references.
 *  2. Remove 'const' and 'volatile'.
 */
template <typename T>
struct Downcast
{
    using Type = std::remove_cv_t<std::remove_reference_t<T>>;
};
template <typename T>
using DowncastType = typename Downcast<T>::Type;

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
