#pragma once

#include <map>
#include <sdbusplus/message.hpp>
#include <string>
#include <variant>

namespace ipmi
{
namespace vpd
{

using Path = std::string;

using Property = std::string;
/// The Value type represents all types that are possible for a FRU info.
/// Most fields in a FRU info are boolean or string. There is also a
/// 3-byte timestamp that, being converted to unix time, fits well into
/// uint64_t.
///
/// However for multirecord area records, there may be other data types,
/// not all of which are directly listed in IPMI FRU specification.
///
/// Hence, this type lists all types possible for sbdusplus except for
/// unixfd, object_path, and signature.
using Value = std::variant<bool, uint8_t, uint16_t, int16_t, uint32_t, int32_t,
                           uint64_t, int64_t, double, std::string>;
using PropertyMap = std::map<Property, Value>;

using Interface = std::string;
using InterfaceMap = std::map<Interface, PropertyMap>;

using Object = sdbusplus::message::object_path;
using ObjectMap = std::map<Object, InterfaceMap>;

} // namespace vpd
} // namespace ipmi
