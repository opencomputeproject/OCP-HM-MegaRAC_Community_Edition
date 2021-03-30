#pragma once
#include <org/open_power/Sensor/Aggregation/History/Maximum/server.hpp>

#include <functional>

namespace witherspoon
{
namespace power
{
namespace history
{

template <typename T>
using ServerObject = typename sdbusplus::server::object::object<T>;

using MaximumInterface =
    sdbusplus::org::open_power::Sensor::Aggregation::History::server::Maximum;

/**
 * @class Maximum
 *
 * Implements Sensor.Aggregation.History.Maximum
 *
 * This includes a property that is an array of timestamp/maximum tuples
 * and a property to specify the scale.
 */
class Maximum : public ServerObject<MaximumInterface>
{
  public:
    static constexpr auto name = "maximum";

    Maximum() = delete;
    Maximum(const Maximum&) = delete;
    Maximum& operator=(const Maximum&) = delete;
    Maximum(Maximum&&) = delete;
    Maximum& operator=(Maximum&&) = delete;
    ~Maximum() = default;

    /**
     * @brief Constructor
     *
     * @param[in] bus - D-Bus object
     * @param[in] objectPath - the D-Bus object path
     */
    Maximum(sdbusplus::bus::bus& bus, const std::string& objectPath) :
        ServerObject<MaximumInterface>(bus, objectPath.c_str())
    {
        unit(Maximum::Unit::Watts);
        scale(0);
    }
};

} // namespace history
} // namespace power
} // namespace witherspoon
