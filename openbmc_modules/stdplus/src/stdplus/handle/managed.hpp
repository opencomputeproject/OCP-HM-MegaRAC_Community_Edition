#pragma once
#include <cstdlib>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace stdplus
{

/** @brief   An RAII handle which takes an object and calls a user specified
 *           function on the object when it should be cleaned up.
 *  @details This is useful for adding RAII semantics to non-RAII things like
 *           file descriptors, or c structs allocated with special routines.
 *           We could make a simple file descriptor wrapper that is
 *           automatically closed:
 *
 *           void closefd(int&& fd) { close(fd); }
 *           using Fd = Managed<int>::Handle<closefd>;
 *
 *           void some_func()
 *           {
 *               Fd fd(open("somefile", 0));
 *               char buf[4096];
 *               int amt = read(*fd, buf, sizeof(buf));
 *               printf("%.*s\n", amt, data);
 *           }
 */
template <typename T, typename... As>
struct Managed
{
    template <void (*drop)(T&&, As&...)>
    class Handle
    {
      public:
        /** @brief Creates a handle owning the object
         *
         *  @param[in] maybeV - Maybe the object being managed
         */
        template <typename... Vs>
        constexpr explicit Handle(std::optional<T>&& maybeV, Vs&&... vs) :
            as(std::forward<Vs>(vs)...), maybeT(std::move(maybeV))
        {
        }
        template <typename... Vs>
        constexpr explicit Handle(T&& maybeV, Vs&&... vs) :
            as(std::forward<Vs>(vs)...), maybeT(std::move(maybeV))
        {
        }

        Handle(const Handle& other) = delete;
        Handle& operator=(const Handle& other) = delete;

        constexpr Handle(Handle&& other) noexcept(
            std::is_nothrow_move_constructible_v<std::tuple<As...>>&&
                std::is_nothrow_move_constructible_v<std::optional<T>>) :
            as(std::move(other.as)),
            maybeT(std::move(other.maybeT))
        {
            other.maybeT = std::nullopt;
        }

        constexpr Handle& operator=(Handle&& other)
        {
            if (this != &other)
            {
                reset(std::move(other.maybeT));
                as = std::move(other.as);
                other.maybeT = std::nullopt;
            }
            return *this;
        }

        virtual ~Handle()
        {
            try
            {
                reset();
            }
            catch (...)
            {
                std::abort();
            }
        }

        /** @brief Gets the managed object
         *
         *  @return A pointer to the object
         */
        constexpr const T* operator->() const noexcept
        {
            return &(*maybeT);
        }

        /** @brief Gets the managed object
         *
         *  @return A reference to the object
         */
        constexpr const T& operator*() const& noexcept
        {
            return *maybeT;
        }

        /** @brief Determine if we are managing an object
         *
         *  @return Do we currently have an object
         */
        constexpr explicit operator bool() const noexcept
        {
            return static_cast<bool>(maybeT);
        }

        /** @brief Determine if we are managing an object
         *
         *  @return Do we currently have an object
         */
        constexpr bool has_value() const noexcept
        {
            return maybeT.has_value();
        }

        /** @brief Gets the managed object
         *
         *  @throws std::bad_optional_access if it has no object
         *  @return A reference to the managed object
         */
        constexpr const T& value() const&
        {
            return maybeT.value();
        }

        /** @brief Gets the managed object if it exists
         *
         *  @throws std::bad_optional_access if it has no object
         *  @return A reference to the managed object
         */
        constexpr const std::optional<T>& maybe_value() const& noexcept
        {
            return maybeT;
        }

        /** @brief Resets the managed value to a new value
         *         The container takes ownership of the value
         *
         *  @param[in] maybeV - Maybe the new value
         */
        constexpr void reset(std::optional<T>&& maybeV)
        {
            maybeDrop(std::index_sequence_for<As...>());
            maybeT = std::move(maybeV);
        }
        constexpr void reset(T&& maybeV)
        {
            maybeDrop(std::index_sequence_for<As...>());
            maybeT = std::move(maybeV);
        }

        /** @brief A shorthand reset function for convenience
         *         Same as calling reset(std::nullopt)
         */
        constexpr void reset()
        {
            reset(std::nullopt);
        }

        /** @brief Releases the managed value and transfers ownership
         *         to the caller.
         *
         *  @throws std::bad_optional_access if it has no object
         *  @return The value that was managed
         */
        [[nodiscard]] constexpr T release()
        {
            T ret = std::move(maybeT.value());
            maybeT = std::nullopt;
            return ret;
        }

        /** @brief Releases the managed value and transfers ownership
         *         to the caller.
         *
         *  @return Maybe the value that was managed
         */
        [[nodiscard]] constexpr std::optional<T> maybe_release() noexcept
        {
            std::optional<T> ret = std::move(maybeT);
            maybeT = std::nullopt;
            return ret;
        }

        /** @brief Reference the contained data
         *
         *  @return A reference to the contained data
         */
        constexpr const std::tuple<As...>& data() const noexcept
        {
            return as;
        }
        constexpr std::tuple<As...>& data() noexcept
        {
            return as;
        }

      protected:
        /* Hold the data parameterized for this container */
        std::tuple<As...> as;

      private:
        /* Stores the managed object if we have one */
        std::optional<T> maybeT;

        template <size_t... Indices>
        void maybeDrop(std::index_sequence<Indices...>)
        {
            if (maybeT)
            {
                drop(std::move(*maybeT), std::get<Indices>(as)...);
            }
        }
    };
};

} // namespace stdplus
