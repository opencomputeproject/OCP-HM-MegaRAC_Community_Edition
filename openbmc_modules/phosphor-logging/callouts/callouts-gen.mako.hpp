## Note that this file is not auto generated, it is what generates the
## callouts-gen.hpp file
// This file was autogenerated.  Do not edit!
// See callouts-gen.py for more details
#pragma once

#include <string>
#include <tuple>

namespace phosphor
{
namespace logging
{

constexpr auto callouts =
{
% for key, value in sorted(calloutsMap.items()):
    std::make_tuple("${key}", "${value}"),
% endfor
};

} // namespace logging
} // namespace phosphor
