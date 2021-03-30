#pragma once

#include "evdevpp/evdev.hpp"
#include "psensor.hpp"
#include "utility.hpp"

#include <sdeventplus/source/io.hpp>

#include <optional>

namespace phosphor
{
namespace fan
{
namespace presence
{
class RedundancyPolicy;

/**
 * @class Gpio
 * @brief Gpio presence sensor implementation.
 *
 * The Gpio class uses a gpio wire to determine presence state.
 */
class Gpio : public PresenceSensor
{
  public:
    /**
     * @brief
     *
     * Cannot move or copy due to this ptr as context
     * for sdevent callbacks.
     */
    Gpio() = delete;
    Gpio(const Gpio&) = delete;
    Gpio& operator=(const Gpio&) = delete;
    Gpio(Gpio&&) = delete;
    Gpio& operator=(Gpio&&) = delete;
    ~Gpio() = default;

    /**
     * @brief Construct a gpio sensor.
     *
     * @param[in] physDevice - The physical gpio device path.
     * @param[in] device - The gpio-keys input device.
     * @param[in] physPin - The physical gpio pin number.
     */
    Gpio(const std::string& physDevice, const std::string& device,
         unsigned int physPin);

    /**
     * @brief start
     *
     * Register for an sdevent io callback on the gpio.
     * Query the initial state of the gpio.
     *
     * @return The current sensor state.
     */
    bool start() override;

    /**
     * @brief stop
     *
     * De-register sdevent io callback.
     */
    void stop() override;

    /**
     * @brief fail
     *
     * Call the gpio out.
     */
    void fail() override;

    /**
     * @brief Check the sensor.
     *
     * Query the gpio.
     */
    bool present() override;

  private:
    /** @brief Get the policy associated with this sensor. */
    virtual RedundancyPolicy& getPolicy() = 0;

    /** @brief sdevent io callback. */
    void ioCallback();

    /** The current state of the sensor. */
    bool currentState;

    /** Gpio event device file descriptor. */
    util::FileDescriptor evdevfd;

    /** Gpio event device. */
    evdevpp::evdev::EvDev evdev;

    /** Physical gpio device. */
    std::string phys;

    /** Gpio pin number. */
    unsigned int pin;

    /** sdevent io handle. */
    std::optional<sdeventplus::source::IO> source;
};

} // namespace presence
} // namespace fan
} // namespace phosphor
