#pragma once

#include <cstring>
#include <sdbusplus/message/types.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace phosphor
{
namespace inventory
{
namespace manager
{
/** @struct MakeVariantVisitor
 *  @brief Return a variant if the visited type is a possible variant type.
 *
 *  @tparam V - The desired variant type.
 */
template <typename V>
struct MakeVariantVisitor
{
    /** @struct Make
     *  @brief Return variant visitor.
     *
     *  @tparam T - The variant type to return.
     *  @tparam Arg - The type being visited in the source variant.
     *  @tparam Enable - Overload resolution removal.
     */
    template <typename T, typename Arg, typename Enable = void>
    struct Make
    {
        static auto make(Arg&& arg)
        {
            throw std::runtime_error(
                "Invalid conversion in MakeVariantVisitor");
            return T();
        }
    };

    /** @struct Make
     *  @brief Return variant visitor.
     *
     *  struct Make specialization if Arg is in T (int -> variant<int, char>).
     */
    template <typename T, typename Arg>
    struct Make<
        T, Arg,
        typename std::enable_if<std::is_convertible<Arg, T>::value>::type>
    {
        static auto make(Arg&& arg)
        {
            return T(std::forward<Arg>(arg));
        }
    };

    /** @brief Make variant visitor.  */
    template <typename Arg>
    auto operator()(Arg&& arg) const
    {
        return Make<V, Arg>::make(arg);
    }
};

/** @brief Convert variants with different contained types.
 *
 *  @tparam V - The desired variant type.
 *  @tparam Arg - The source variant type.
 *
 *  @param[in] v - The source variant.
 *  @returns - The converted variant.
 */
template <typename V, typename Arg>
auto convertVariant(Arg&& v)
{
    return std::visit(MakeVariantVisitor<V>(), v);
}

/** @struct CompareFirst
 *  @brief std::pair binary comparison adapter.
 *
 *  Adapt a binary comparison function to a comparison of
 *  the first pair element.
 *
 *  @tparam Compare - The function object type being adapted.
 */
template <typename Compare>
struct CompareFirst
{
    /** @brief Construct a CompareFirst adapter.
     *
     *  @param[in] c - The function object being adapted.
     */
    explicit CompareFirst(Compare&& c) : compare(std::forward<Compare>(c))
    {
    }

    /** @brief Compare two pairs adapter.
     *
     *  @tparam L1 - First pair first_type.
     *  @tparam L2 - First pair second_type.
     *  @tparam R1 - Second pair first_type, convertible to L1.
     *  @tparam R2 - Second pair second_type.
     *
     *  @param[in] l - The first pair.
     *  @param[in] r - The second pair.
     *
     *  @returns - The result of the comparison.
     */
    template <typename L1, typename L2, typename R1, typename R2>
    bool operator()(const std::pair<L1, L2>& l,
                    const std::pair<R1, R2>& r) const
    {
        return compare(l.first, r.first);
    }

    /** @brief Compare one pair adapter.
     *
     *  @tparam L1 - Pair first_type.
     *  @tparam L2 - Pair second_type.
     *  @tparam R - Convertible to L1 for comparison.
     *
     *  @param[in] l - The pair.
     *  @param[in] r - To be compared to l.first.
     *
     *  @returns - The result of the comparison.
     */
    template <typename L1, typename L2, typename R>
    bool operator()(const std::pair<L1, L2>& l, const R& r) const
    {
        return compare(l.first, r);
    }

    /** @brief Compare one pair adapter.
     *
     *  @tparam L - Convertible to R1 for comparison.
     *  @tparam R1 - Pair first_type.
     *  @tparam R2 - Pair second_type.
     *
     *  @param[in] l - To be compared to r.first.
     *  @param[in] r - The pair.
     *
     *  @returns - The result of the comparison.
     */
    template <typename L, typename R1, typename R2>
    bool operator()(const L& l, const std::pair<R1, R2>& r) const
    {
        return compare(l, r.first);
    }

    /* @brief The function being adapted. */
    Compare compare;
};

/* @brief Implicit template instantation wrapper for CompareFirst. */
template <typename Compare>
CompareFirst<Compare> compareFirst(Compare&& c)
{
    return CompareFirst<Compare>(std::forward<Compare>(c));
}

/** @struct RelPathCompare
 *  @brief Compare two strings after removing an optional prefix.
 */
struct RelPathCompare
{
    /** @brief Construct a RelPathCompare comparison functor.
     *
     *  @param[in] p - The prefix to check for and remove.
     */
    explicit RelPathCompare(const char* p) : prefix(p)
    {
    }

    /** @brief Check for the prefix and remove if found.
     *
     *  @param[in] s - The string to check for and remove prefix from.
     */
    auto relPath(const std::string& s) const
    {
        if (s.find(prefix) == 0)
        {
            return s.substr(strlen(prefix));
        }

        return s;
    }

    /** @brief Comparison method.
     *
     *  @param[in] l - The first string.
     *  @param[in] r - The second string.
     *
     *  @returns - The result of the comparison.
     */
    bool operator()(const std::string& l, const std::string& r) const
    {
        return relPath(l) < relPath(r);
    }

    /* The path prefix to remove when comparing two paths. */
    const char* prefix;
};
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
