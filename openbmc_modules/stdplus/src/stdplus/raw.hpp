#pragma once
#include <fmt/format.h>
#include <stdexcept>
#include <stdplus/types.hpp>
#include <string_view>
#include <type_traits>

namespace stdplus
{
namespace raw
{

namespace detail
{

/** @brief Gets the datatype referenced in a container
 */
template <typename Container>
using dataType = std::remove_pointer_t<decltype(
    std::data(std::declval<std::add_lvalue_reference_t<Container>>()))>;

/** @brief Determines if the container holds trivially copyable data
 */
template <typename Container>
inline constexpr bool trivialContainer =
    std::is_trivially_copyable_v<dataType<Container>>;

/** @brief Adds const to A if B is const
 */
template <typename A, typename B>
using copyConst =
    std::conditional_t<std::is_const_v<B>, std::add_const_t<A>, A>;

/** @brief Determines if a type is a container of data
 */
template <typename, typename = void>
inline constexpr bool hasData = false;
template <typename T>
inline constexpr bool hasData<T, std::void_t<dataType<T>>> = true;

} // namespace detail

/** @brief Compares two containers to see if their raw bytes are equal
 *
 *  @param[in] a - The first container
 *  @param[in] b - The second container
 *  @return True if they are the same, false otherwise
 */
template <typename A, typename B>
bool equal(const A& a, const B& b)
{
    static_assert(std::is_trivially_copyable_v<A>);
    static_assert(std::is_trivially_copyable_v<B>);
    static_assert(sizeof(A) == sizeof(B));
    return memcmp(&a, &b, sizeof(A)) == 0;
}

/** @brief Copies data from a buffer into a copyable type
 *
 *  @param[in] data - The data buffer being copied from
 *  @return The copyable type with data populated
 */
template <typename T, typename Container>
T copyFrom(const Container& c)
{
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(detail::trivialContainer<Container>);
    T ret;
    const size_t bytes = std::size(c) * sizeof(*std::data(c));
    if (bytes < sizeof(ret))
    {
        throw std::runtime_error(
            fmt::format("CopyFrom: {} < {}", bytes, sizeof(ret)));
    }
    std::memcpy(&ret, std::data(c), sizeof(ret));
    return ret;
}

/** @brief References the data from a buffer if aligned
 *
 *  @param[in] data - The data buffer being referenced
 *  @return The reference to the data in the new type
 */
template <typename T, typename Container,
          typename Tp = detail::copyConst<T, detail::dataType<Container>>>
Tp& refFrom(Container&& c)
{
    static_assert(std::is_trivially_copyable_v<Tp>);
    static_assert(detail::trivialContainer<Container>);
    static_assert(sizeof(*std::data(c)) % alignof(Tp) == 0);
    const size_t bytes = std::size(c) * sizeof(*std::data(c));
    if (bytes < sizeof(Tp))
    {
        throw std::runtime_error(
            fmt::format("RefFrom: {} < {}", bytes, sizeof(Tp)));
    }
    return *reinterpret_cast<Tp*>(std::data(c));
}

/** @brief Extracts data from a buffer into a copyable type
 *         Updates the data buffer to show that data was removed
 *
 *  @param[in,out] data - The data buffer being extracted from
 *  @return The copyable type with data populated
 */
template <typename T, typename CharT>
T extract(std::basic_string_view<CharT>& data)
{
    T ret = copyFrom<T>(data);
    static_assert(sizeof(T) % sizeof(CharT) == 0);
    data.remove_prefix(sizeof(T) / sizeof(CharT));
    return ret;
}
#ifdef STDPLUS_SPAN_TYPE
template <typename T, typename IntT,
          typename = std::enable_if_t<std::is_trivially_copyable_v<IntT>>>
T extract(span<IntT>& data)
{
    T ret = copyFrom<T>(data);
    static_assert(sizeof(T) % sizeof(IntT) == 0);
    data = data.subspan(sizeof(T) / sizeof(IntT));
    return ret;
}
#endif

/** @brief Extracts data from a buffer as a reference if aligned
 *         Updates the data buffer to show that data was removed
 *
 *  @param[in,out] data - The data buffer being extracted from
 *  @return A reference to the data
 */
template <typename T, typename CharT>
const T& extractRef(std::basic_string_view<CharT>& data)
{
    const T& ret = refFrom<T>(data);
    static_assert(sizeof(T) % sizeof(CharT) == 0);
    data.remove_prefix(sizeof(T) / sizeof(CharT));
    return ret;
}
#ifdef STDPLUS_SPAN_TYPE
template <typename T, typename IntT,
          typename = std::enable_if_t<std::is_trivially_copyable_v<IntT>>,
          typename Tp = detail::copyConst<T, IntT>>
Tp& extractRef(span<IntT>& data)
{
    Tp& ret = refFrom<Tp>(data);
    static_assert(sizeof(Tp) % sizeof(IntT) == 0);
    data = data.subspan(sizeof(Tp) / sizeof(IntT));
    return ret;
}
#endif

/** @brief Returns the span referencing the data of the raw trivial type
 *         or of trivial types in a contiguous container.
 *
 *  @param[in] t - The trivial raw data
 *  @return A view over the input with the given output integral type
 */
template <typename CharT, typename T>
std::enable_if_t<!detail::hasData<T>, std::basic_string_view<CharT>>
    asView(const T& t) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) % sizeof(CharT) == 0);
    return {reinterpret_cast<const CharT*>(&t), sizeof(T) / sizeof(CharT)};
}

template <typename CharT, typename Container>
std::enable_if_t<detail::hasData<Container>, std::basic_string_view<CharT>>
    asView(const Container& c) noexcept
{
    static_assert(detail::trivialContainer<Container>);
    static_assert(sizeof(*std::data(c)) % sizeof(CharT) == 0);
    return {reinterpret_cast<const CharT*>(std::data(c)),
            std::size(c) * sizeof(*std::data(c)) / sizeof(CharT)};
}

#ifdef STDPLUS_SPAN_TYPE
template <typename IntT, typename T,
          typename = std::enable_if_t<std::is_trivially_copyable_v<IntT>>,
          typename = std::enable_if_t<!detail::hasData<T>>,
          typename IntTp = detail::copyConst<IntT, T>>
span<IntTp> asSpan(T& t) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) % sizeof(IntTp) == 0);
    return {reinterpret_cast<IntTp*>(&t), sizeof(T) / sizeof(IntTp)};
}
template <typename IntT, typename Container,
          typename = std::enable_if_t<std::is_trivially_copyable_v<IntT>>,
          typename = std::enable_if_t<detail::hasData<Container>>,
          typename IntTp = detail::copyConst<IntT, detail::dataType<Container>>>
span<IntTp> asSpan(Container&& c) noexcept
{
    static_assert(detail::trivialContainer<Container>);
    static_assert(sizeof(*std::data(c)) % sizeof(IntTp) == 0);
    return {reinterpret_cast<IntTp*>(std::data(c)),
            std::size(c) * sizeof(*std::data(c)) / sizeof(IntTp)};
}
#endif

} // namespace raw
} // namespace stdplus
