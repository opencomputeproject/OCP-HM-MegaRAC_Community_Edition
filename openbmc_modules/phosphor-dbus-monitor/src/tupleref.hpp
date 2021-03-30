#pragma once

#include <cstddef>
#include <tuple>
#include <utility>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/** @brief A tuple of references. */
template <typename... T>
using TupleOfRefs = std::tuple<std::reference_wrapper<T>...>;

namespace detail
{
/** @brief Less than implementation for tuples of references. */
template <size_t size, size_t i, typename T, typename U>
struct TupleOfRefsLess
{
    static constexpr bool compare(const T& l, const U& r)
    {
        if (std::get<i>(l).get() < std::get<i>(r).get())
        {
            return true;
        }
        if (std::get<i>(r).get() < std::get<i>(l).get())
        {
            return false;
        }
        return TupleOfRefsLess<size, i + 1, T, U>::compare(l, r);
    }
};

/** @brief Less than specialization for tuple element sizeof...(tuple) +1. */
template <size_t size, typename T, typename U>
struct TupleOfRefsLess<size, size, T, U>
{
    static constexpr bool compare(const T& l, const U& r)
    {
        return false;
    }
};
} // namespace detail

/** @brief Less than comparison for tuples of references. */
struct TupleOfRefsLess
{
    template <typename... T, typename... U>
    constexpr bool operator()(const TupleOfRefs<T...>& l,
                              const TupleOfRefs<U...>& r) const
    {
        static_assert(sizeof...(T) == sizeof...(U),
                      "Cannot compare tuples of different lengths.");
        return detail::TupleOfRefsLess<sizeof...(T), 0, TupleOfRefs<T...>,
                                       TupleOfRefs<U...>>::compare(l, r);
    }
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
