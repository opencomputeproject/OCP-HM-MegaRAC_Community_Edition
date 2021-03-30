#pragma once
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

namespace stdplus
{
namespace util
{
namespace detail
{

template <typename... Views>
void strAppendViews(std::string& dst, Views... views)
{
    dst.reserve((dst.size() + ... + views.size()));
    (dst.append(views), ...);
}

} // namespace detail

/** @brief Appends multiple strings to the end of the destination string
 *         in the most optimal way for the given inputs.
 *
 *  @param[in, out] dst - The string being appended to
 *  @param[in] ...strs  - An arbitrary number of strings to concatenate
 */
template <typename... Strs>
void strAppend(std::string& dst, const Strs&... strs)
{
    detail::strAppendViews(dst, std::string_view(strs)...);
}

/** @brief Concatenates multiple strings together in the most optimal
 *         way for the given inputs.
 *
 *  @param[in] ...strs - An arbitrary number of strings to concatenate
 *  @return The concatenated result string
 */
template <typename... Strs>
std::string strCat(const Strs&... strs)
{
    std::string ret;
    strAppend(ret, strs...);
    return ret;
}
template <typename... Strs>
std::string strCat(std::string&& in, const Strs&... strs)
{
    std::string ret = std::move(in);
    strAppend(ret, strs...);
    return ret;
}

} // namespace util
} // namespace stdplus
