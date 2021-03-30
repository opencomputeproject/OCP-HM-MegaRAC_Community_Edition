#pragma once

#include "sensor.hpp"

#include <memory>
#include <mutex>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <type_traits>
#include <xyz/openbmc_project/Sensor/Value/server.hpp>

template <typename... T>
using ServerObject = typename sdbusplus::server::object::object<T...>;

using ValueInterface = sdbusplus::xyz::openbmc_project::Sensor::server::Value;
using ValueObject = ServerObject<ValueInterface>;

class ValueHelper : public ValueInterface
{

  public:
    auto operator()() const
    {
        return value();
    }
};

constexpr bool usingDouble =
    std::is_same_v<std::result_of_t<ValueHelper()>, double>;
using ValueType = std::conditional_t<usingDouble, double, int64_t>;

/*
 * HostSensor object is a Sensor derivative that also implements a ValueObject,
 * which comes from the dbus as an object that implements Sensor.Value.
 */
class HostSensor : public Sensor, public ValueObject
{
  public:
    static std::unique_ptr<Sensor> createTemp(const std::string& name,
                                              int64_t timeout,
                                              sdbusplus::bus::bus& bus,
                                              const char* objPath, bool defer);

    HostSensor(const std::string& name, int64_t timeout,
               sdbusplus::bus::bus& bus, const char* objPath, bool defer) :
        Sensor(name, timeout),
        ValueObject(bus, objPath, defer)
    {
    }

    ValueType value(ValueType value) override;

    ReadReturn read(void) override;
    void write(double value) override;

  private:
    /*
     * _lock will be used to make sure _updated & _value are updated
     * together.
     */
    std::mutex _lock;
    std::chrono::high_resolution_clock::time_point _updated;
    double _value = 0;
};
