## This file is a template, the comment below is emitted into the generated file
/* This is an auto generated file. Do not edit. */
#pragma once

#include <array>
#include <memory>
#include <string>
#include "anyof.hpp"
#include "fallback.hpp"
#include "fan.hpp"
#include "gpio.hpp"
#include "tach.hpp"

using namespace std::string_literals;

namespace phosphor
{
namespace fan
{
namespace presence
{

struct ConfigPolicy;

struct ConfigSensors
{
    using Sensors = std::array<std::unique_ptr<PresenceSensor>, ${len(sensors)}>;

    static auto& get()
    {
        static const Sensors sensors =
        {
% for s in sensors:
            ${s.construct(loader, indent=indent +3)},
% endfor
        };
        return sensors;
    }
};

struct ConfigFans
{
    using Fans = std::array<Fan, ${len(fans)}>;

    static auto& get()
    {
        static const Fans fans =
        {
            {
% for f in fans:
                Fans::value_type{
                    "${f.name}"s,
                    "${f.path}"s,
                },
% endfor
            }
        };
        return fans;
    }
};

struct ConfigPolicy
{
    using Policies = std::array<std::unique_ptr<RedundancyPolicy>, ${len(policies)}>;

    static auto& get()
    {
        static const Policies policies =
        {
% for p in policies:
            ${p.construct(loader, indent=indent +3)},
% endfor
        };
        return policies;
    }
};
} // namespace presence
} // namespace fan
} // namespace phosphor
