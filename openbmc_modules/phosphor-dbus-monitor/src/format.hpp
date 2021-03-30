#pragma once

#include "data_types.hpp"

#include <string>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{
namespace detail
{

/** @brief Map format strings to undecorated C++ types. */
template <typename T>
struct GetFormatType
{
};
template <>
struct GetFormatType<bool>
{
    static constexpr auto format = "%d";
};
template <>
struct GetFormatType<char>
{
    static constexpr auto format = "=%hhd";
};
template <>
struct GetFormatType<short int>
{
    static constexpr auto format = "=%hd";
};
template <>
struct GetFormatType<int>
{
    static constexpr auto format = "=%d";
};
template <>
struct GetFormatType<long int>
{
    static constexpr auto format = "=%ld";
};
template <>
struct GetFormatType<long long int>
{
    static constexpr auto format = "=%lld";
};
template <>
struct GetFormatType<unsigned char>
{
    static constexpr auto format = "=%hhd";
};
template <>
struct GetFormatType<unsigned short int>
{
    static constexpr auto format = "=%hd";
};
template <>
struct GetFormatType<unsigned int>
{
    static constexpr auto format = "=%d";
};
template <>
struct GetFormatType<unsigned long int>
{
    static constexpr auto format = "=%ld";
};
template <>
struct GetFormatType<unsigned long long int>
{
    static constexpr auto format = "=%lld";
};
template <>
struct GetFormatType<std::string>
{
    static constexpr auto format = "=%s";
};
template <>
struct GetFormatType<char*>
{
    static constexpr auto format = "=%s";
};
template <>
struct GetFormatType<const char*>
{
    static constexpr auto format = "=%s";
};

} // namespace detail

/** @brief Get the format string for a C++ type. */
template <typename T>
struct GetFormat
{
    static constexpr auto format =
        detail::GetFormatType<DowncastType<T>>::format;
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
