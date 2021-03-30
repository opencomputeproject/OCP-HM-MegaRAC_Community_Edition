#pragma once

#include "Thresholds.hpp"
#include "Utils.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <limits>
#include <memory>
#include <string>
#include <vector>

constexpr size_t sensorFailedPollTimeMs = 5000;

constexpr const char* sensorValueInterface = "xyz.openbmc_project.Sensor.Value";
constexpr const char* availableInterfaceName =
    "xyz.openbmc_project.State.Decorator.Availability";
constexpr const char* operationalInterfaceName =
    "xyz.openbmc_project.State.Decorator.OperationalStatus";
constexpr const size_t errorThreshold = 5;

struct Sensor
{
    Sensor(const std::string& name,
           std::vector<thresholds::Threshold>&& thresholdData,
           const std::string& configurationPath, const std::string& objectType,
           const double max, const double min,
           PowerState readState = PowerState::always) :
        name(std::regex_replace(name, std::regex("[^a-zA-Z0-9_/]+"), "_")),
        configurationPath(configurationPath), objectType(objectType),
        maxValue(max), minValue(min), thresholds(std::move(thresholdData)),
        hysteresisTrigger((max - min) * 0.01),
        hysteresisPublish((max - min) * 0.0001), readState(readState),
        errCount(0)
    {}
    virtual ~Sensor() = default;
    virtual void checkThresholds(void) = 0;
    std::string name;
    std::string configurationPath;
    std::string objectType;
    double maxValue;
    double minValue;
    std::vector<thresholds::Threshold> thresholds;
    std::shared_ptr<sdbusplus::asio::dbus_interface> sensorInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> thresholdInterfaceWarning;
    std::shared_ptr<sdbusplus::asio::dbus_interface> thresholdInterfaceCritical;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;
    std::shared_ptr<sdbusplus::asio::dbus_interface> availableInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> operationalInterface;
    double value = std::numeric_limits<double>::quiet_NaN();
    bool overriddenState = false;
    bool internalSet = false;
    double hysteresisTrigger;
    double hysteresisPublish;
    PowerState readState;
    size_t errCount;

    int setSensorValue(const double& newValue, double& oldValue)
    {
        if (!internalSet)
        {
            oldValue = newValue;
            overriddenState = true;
            // check thresholds for external set
            value = newValue;
            checkThresholds();
        }
        else if (!overriddenState)
        {
            oldValue = newValue;
        }
        return 1;
    }

    void
        setInitialProperties(std::shared_ptr<sdbusplus::asio::connection>& conn,
                             const std::string label = std::string(),
                             size_t thresholdSize = 0)
    {
        if (readState == PowerState::on || readState == PowerState::biosPost)
        {
            setupPowerMatch(conn);
        }

        createAssociation(association, configurationPath);

        sensorInterface->register_property("MaxValue", maxValue);
        sensorInterface->register_property("MinValue", minValue);
        sensorInterface->register_property(
            "Value", value, [&](const double& newValue, double& oldValue) {
                return setSensorValue(newValue, oldValue);
            });
        for (auto& threshold : thresholds)
        {
            std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
            std::string level;
            std::string alarm;
            if (threshold.level == thresholds::Level::CRITICAL)
            {
                iface = thresholdInterfaceCritical;
                if (threshold.direction == thresholds::Direction::HIGH)
                {
                    level = "CriticalHigh";
                    alarm = "CriticalAlarmHigh";
                }
                else
                {
                    level = "CriticalLow";
                    alarm = "CriticalAlarmLow";
                }
            }
            else if (threshold.level == thresholds::Level::WARNING)
            {
                iface = thresholdInterfaceWarning;
                if (threshold.direction == thresholds::Direction::HIGH)
                {
                    level = "WarningHigh";
                    alarm = "WarningAlarmHigh";
                }
                else
                {
                    level = "WarningLow";
                    alarm = "WarningAlarmLow";
                }
            }
            else
            {
                std::cerr << "Unknown threshold level" << threshold.level
                          << "\n";
                continue;
            }
            if (!iface)
            {
                std::cout << "trying to set uninitialized interface\n";
                continue;
            }

            size_t thresSize =
                label.empty() ? thresholds.size() : thresholdSize;
            iface->register_property(
                level, threshold.value,
                [&, label, thresSize](const double& request, double& oldValue) {
                    oldValue = request; // todo, just let the config do this?
                    threshold.value = request;
                    thresholds::persistThreshold(configurationPath, objectType,
                                                 threshold, conn, thresSize,
                                                 label);
                    // Invalidate previously remembered value,
                    // so new thresholds will be checked during next update,
                    // even if sensor reading remains unchanged.
                    value = std::numeric_limits<double>::quiet_NaN();

                    // Although tempting, don't call checkThresholds() from here
                    // directly. Let the regular sensor monitor call the same
                    // using updateValue(), which can check conditions like
                    // poweron, etc., before raising any event.
                    return 1;
                });
            iface->register_property(alarm, false);
        }
        if (!sensorInterface->initialize())
        {
            std::cerr << "error initializing value interface\n";
        }
        if (thresholdInterfaceWarning &&
            !thresholdInterfaceWarning->initialize(true))
        {
            std::cerr << "error initializing warning threshold interface\n";
        }

        if (thresholdInterfaceCritical &&
            !thresholdInterfaceCritical->initialize(true))
        {
            std::cerr << "error initializing critical threshold interface\n";
        }

        if (!availableInterface)
        {
            availableInterface =
                std::make_shared<sdbusplus::asio::dbus_interface>(
                    conn, sensorInterface->get_object_path(),
                    availableInterfaceName);
            availableInterface->register_property(
                "Available", true, [this](const bool propIn, bool& old) {
                    if (propIn == old)
                    {
                        return 1;
                    }
                    old = propIn;
                    if (!propIn)
                    {
                        updateValue(std::numeric_limits<double>::quiet_NaN());
                    }
                    return 1;
                });
            availableInterface->initialize();
        }
        if (!operationalInterface)
        {
            operationalInterface =
                std::make_shared<sdbusplus::asio::dbus_interface>(
                    conn, sensorInterface->get_object_path(),
                    operationalInterfaceName);
            operationalInterface->register_property("Functional", true);
            operationalInterface->initialize();
        }
    }

    bool readingStateGood()
    {
        if (readState == PowerState::on && !isPowerOn())
        {
            return false;
        }
        if (readState == PowerState::biosPost &&
            (!hasBiosPost() || !isPowerOn()))
        {
            return false;
        }

        return true;
    }

    void markFunctional(bool isFunctional)
    {
        if (operationalInterface)
        {
            operationalInterface->set_property("Functional", isFunctional);
        }
        if (isFunctional)
        {
            errCount = 0;
        }
        else
        {
            updateValue(std::numeric_limits<double>::quiet_NaN());
        }
    }

    void markAvailable(bool isAvailable)
    {
        if (availableInterface)
        {
            availableInterface->set_property("Available", isAvailable);
            errCount = 0;
        }
    }

    void incrementError()
    {
        if (!readingStateGood())
        {
            markAvailable(false);
            return;
        }

        if (errCount >= errorThreshold)
        {
            return;
        }

        errCount++;
        if (errCount == errorThreshold)
        {
            std::cerr << "Sensor " << name << " reading error!\n";
            markFunctional(false);
        }
    }

    void updateValue(const double& newValue)
    {
        // Ignore if overriding is enabled
        if (overriddenState)
        {
            return;
        }

        if (!readingStateGood())
        {
            markAvailable(false);
            return;
        }

        // Indicate that it is internal set call
        internalSet = true;
        updateProperty(sensorInterface, value, newValue, "Value");
        internalSet = false;

        // Always check thresholds after changing the value,
        // as the test against hysteresisTrigger now takes place in
        // the thresholds::checkThresholds() method,
        // which is called by checkThresholds() below,
        // in all current implementations of sensors that have thresholds.
        checkThresholds();
        if (!std::isnan(newValue))
        {
            markFunctional(true);
            markAvailable(true);
        }
    }

    void updateProperty(
        std::shared_ptr<sdbusplus::asio::dbus_interface>& interface,
        double& oldValue, const double& newValue, const char* dbusPropertyName)
    {
        if (requiresUpdate(oldValue, newValue))
        {
            oldValue = newValue;
            if (!(interface->set_property(dbusPropertyName, newValue)))
            {
                std::cerr << "error setting property " << dbusPropertyName
                          << " to " << newValue << "\n";
            }
        }
    }

    bool requiresUpdate(const double& lVal, const double& rVal)
    {
        if (std::isnan(lVal) || std::isnan(rVal))
        {
            return true;
        }
        double diff = std::abs(lVal - rVal);
        if (diff > hysteresisPublish)
        {
            return true;
        }
        return false;
    }
};
