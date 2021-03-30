#pragma once
#include <optional>
#include <stdplus/handle/managed.hpp>
#include <type_traits>
#include <utility>

namespace stdplus
{

/** @brief Similar to the Managed Handle, but also allows for copying
 *         and performs an operation during that copy.
 */
template <typename T, typename... As>
struct Copyable
{
    template <void (*drop)(T&&, As&...), T (*ref)(const T&, As&...)>
    class Handle : public Managed<T, As...>::template Handle<drop>
    {
      public:
        using MHandle = typename Managed<T, As...>::template Handle<drop>;

        /** @brief Creates a handle referencing the object
         *
         *  @param[in] maybeV - Optional object being managed
         */
        template <typename... Vs>
        constexpr explicit Handle(const std::optional<T>& maybeV, Vs&&... vs) :
            MHandle(std::nullopt, std::forward<Vs>(vs)...)
        {
            reset(maybeV);
        }
        template <typename... Vs>
        constexpr explicit Handle(const T& maybeV, Vs&&... vs) :
            MHandle(std::nullopt, std::forward<Vs>(vs)...)
        {
            reset(maybeV);
        }

        /** @brief Creates a handle owning the object
         *
         *  @param[in] maybeV - Maybe the object being managed
         */
        template <typename... Vs>
        constexpr explicit Handle(std::optional<T>&& maybeV, Vs&&... vs) :
            MHandle(std::move(maybeV), std::forward<Vs>(vs)...)
        {
        }
        template <typename... Vs>
        constexpr explicit Handle(T&& maybeV, Vs&&... vs) :
            MHandle(std::move(maybeV), std::forward<Vs>(vs)...)
        {
        }

        constexpr Handle(const Handle& other) : MHandle(std::nullopt, other.as)
        {
            reset(other.maybe_value());
        }

        constexpr Handle(Handle&& other) noexcept(
            std::is_nothrow_move_constructible_v<MHandle>) :
            MHandle(std::move(other))
        {
        }

        constexpr Handle& operator=(const Handle& other)
        {
            if (this != &other)
            {
                reset();
                this->as = other.as;
                reset(other.maybe_value());
            }
            return *this;
        }

        constexpr Handle& operator=(Handle&& other)
        {
            MHandle::operator=(std::move(other));
            return *this;
        }

        using MHandle::reset;

        /** @brief Resets the managed value to a new value
         *         Takes a new reference on the value
         *
         *  @param[in] maybeV - Maybe the new value
         */
        constexpr void reset(const std::optional<T>& maybeV)
        {
            if (maybeV)
            {
                reset(doRef(*maybeV, std::index_sequence_for<As...>()));
            }
            else
            {
                reset(std::nullopt);
            }
        }
        constexpr void reset(const T& maybeV)
        {
            reset(doRef(maybeV, std::index_sequence_for<As...>()));
        }

      private:
        template <size_t... Indices>
        T doRef(const T& v, std::index_sequence<Indices...>)
        {
            return ref(v, std::get<Indices>(this->as)...);
        }
    };
};

} // namespace stdplus
