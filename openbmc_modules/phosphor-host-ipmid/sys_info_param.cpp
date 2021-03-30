#include "sys_info_param.hpp"

std::tuple<bool, std::string>
    SysInfoParamStore::lookup(uint8_t paramSelector) const
{
    const auto iterator = params.find(paramSelector);
    if (iterator == params.end())
    {
        return std::make_tuple(false, "");
    }

    auto& callback = iterator->second;
    auto s = callback();
    return std::make_tuple(true, s);
}

void SysInfoParamStore::update(uint8_t paramSelector, const std::string& s)
{
    // Add a callback that captures a copy of the string passed and returns it
    // when invoked.

    // clang-format off
    update(paramSelector, [s]() {
        return s;
    });
    // clang-format on
}

void SysInfoParamStore::update(uint8_t paramSelector,
                               const std::function<std::string()>& callback)
{
    params[paramSelector] = callback;
}
