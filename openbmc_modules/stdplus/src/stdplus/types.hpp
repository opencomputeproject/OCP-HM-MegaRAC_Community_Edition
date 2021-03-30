#pragma once
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
#include <span>
#define STDPLUS_SPAN_TYPE std::span
#elif __has_include(<nonstd/span.hpp>)
#include <nonstd/span.hpp>
#define STDPLUS_SPAN_TYPE nonstd::span
#endif

namespace stdplus
{

#ifdef STDPLUS_SPAN_TYPE
template <typename... Args>
using span = STDPLUS_SPAN_TYPE<Args...>;
#endif

} // namespace stdplus
