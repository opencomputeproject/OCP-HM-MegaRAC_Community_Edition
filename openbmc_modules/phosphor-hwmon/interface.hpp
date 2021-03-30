#pragma once

#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Control/FanPwm/server.hpp>
#include <xyz/openbmc_project/Control/FanSpeed/server.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Critical/server.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Warning/server.hpp>
#include <xyz/openbmc_project/Sensor/Value/server.hpp>
#include <xyz/openbmc_project/State/Decorator/OperationalStatus/server.hpp>

template <typename... T>
using ServerObject = typename sdbusplus::server::object::object<T...>;

using ValueInterface = sdbusplus::xyz::openbmc_project::Sensor::server::Value;
using ValueObject = ServerObject<ValueInterface>;
using WarningInterface =
    sdbusplus::xyz::openbmc_project::Sensor::Threshold::server::Warning;
using WarningObject = ServerObject<WarningInterface>;
using CriticalInterface =
    sdbusplus::xyz::openbmc_project::Sensor::Threshold::server::Critical;
using CriticalObject = ServerObject<CriticalInterface>;
using FanSpeedInterface =
    sdbusplus::xyz::openbmc_project::Control::server::FanSpeed;
using FanSpeedObject = ServerObject<FanSpeedInterface>;
using FanPwmInterface =
    sdbusplus::xyz::openbmc_project::Control::server::FanPwm;
using FanPwmObject = ServerObject<FanPwmInterface>;
using StatusInterface = sdbusplus::xyz::openbmc_project::State::Decorator::
    server::OperationalStatus;
using StatusObject = ServerObject<StatusInterface>;

// I understand this seems like magic, but since decltype doesn't evaluate you
// can call nullptr https://stackoverflow.com/a/5580411/2784885
using SensorValueType =
    decltype((static_cast<ValueInterface*>(nullptr))->value());

enum class InterfaceType
{
    VALUE,
    WARN,
    CRIT,
    FAN_SPEED,
    FAN_PWM,
    STATUS,
};

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
