#pragma once

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <exception>
#include <functional>
#include <sdeventplus/exception.hpp>
#include <stdexcept>
#include <stdplus/util/cexec.hpp>
#include <utility>

namespace sdeventplus
{

// Defined by systemd taking uint64_t usec params
using SdEventDuration =
    std::chrono::duration<uint64_t, std::chrono::microseconds::period>;

namespace internal
{

/** @brief Handle sd_event callback exception gracefully
 *  @details A generic wrapper that turns exceptions into
 *           error messages and return codes.
 */
template <typename... Args>
inline int performCallback(const char* name, Args&&... args)
{
    try
    {
        std::invoke(std::forward<Args>(args)...);
        return 0;
    }
    catch (const std::system_error& e)
    {
        fprintf(stderr, "sdeventplus: %s: %s\n", name, e.what());
        return -e.code().value();
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "sdeventplus: %s: %s\n", name, e.what());
        return -ENOSYS;
    }
    catch (...)
    {
        fprintf(stderr, "sdeventplus: %s: Unknown error\n", name);
        return -ENOSYS;
    }
}

/** @brief Constructs an SdEventError for stdplus cexec
 */
inline SdEventError makeError(int error, const char* msg)
{
    return SdEventError(error, msg);
}

template <typename... Args>
inline auto callCheck(const char* msg, Args&&... args)
{
    return stdplus::util::callCheckRet<makeError, Args...>(
        msg, std::forward<Args>(args)...);
}

} // namespace internal
} // namespace sdeventplus
