#pragma once

#include "psensor.hpp"
#include "sdbusplus.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <string>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace presence
{
class RedundancyPolicy;

/**
 * @class Tach
 * @brief Fan tach sensor presence implementation.
 *
 * The Tach class uses one or more tach speed indicators
 * to determine presence state.
 */
class Tach : public PresenceSensor
{
  public:
    /**
     * @brief
     *
     * Cannot move or copy due to this ptr as context
     * for sdbus callbacks.
     */
    Tach() = delete;
    Tach(const Tach&) = delete;
    Tach& operator=(const Tach&) = delete;
    Tach(Tach&&) = delete;
    Tach& operator=(Tach&&) = delete;
    ~Tach() = default;

    /**
     * @brief ctor
     *
     * @param[in] sensors - Fan tach sensors for this psensor.
     */
    Tach(const std::vector<std::string>& sensors);

    /**
     * @brief start
     *
     * Register for dbus signal callbacks on fan
     * tach sensor change.  Query initial tach speeds.
     *
     * @return The current sensor state.
     */
    bool start() override;

    /**
     * @brief stop
     *
     * De-register dbus signal callbacks.
     */
    void stop() override;

    /**
     * @brief Check the sensor.
     *
     * Query the tach speeds.
     */
    bool present() override;

  private:
    /**
     * @brief Get the policy associated with this sensor.
     */
    virtual RedundancyPolicy& getPolicy() = 0;

    /**
     * @brief Properties changed handler for tach sensor updates.
     *
     * @param[in] sensor - The sensor that changed.
     * @param[in] props - The properties that changed.
     */
    void propertiesChanged(
        size_t sensor, const phosphor::fan::util::Properties<int64_t>& props);

    /**
     * @brief Properties changed handler for tach sensor updates.
     *
     * @param[in] sensor - The sensor that changed.
     * @param[in] msg - The sdbusplus signal message.
     */
    void propertiesChanged(size_t sensor, sdbusplus::message::message& msg);

    /** @brief array of tach sensors dbus matches, and tach values. */
    std::vector<std::tuple<
        std::string, std::unique_ptr<sdbusplus::bus::match::match>, int64_t>>
        state;

    /** The current state of the sensor. */
    bool currentState;
};

} // namespace presence
} // namespace fan
} // namespace phosphor
