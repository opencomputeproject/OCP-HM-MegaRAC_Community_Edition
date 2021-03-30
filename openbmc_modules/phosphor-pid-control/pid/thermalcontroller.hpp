#pragma once

#include "ec/pid.hpp"
#include "pidcontroller.hpp"

#include <memory>
#include <string>
#include <vector>

/*
 * A ThermalController is a PID controller that reads a number of sensors and
 * provides the setpoints for the fans.
 */

enum class ThermalType
{
    margin,
    absolute
};

/**
 * Get the ThermalType for a given string.
 *
 * @param[in] typeString - a string representation of a type.
 * @return the ThermalType representation.
 */
ThermalType getThermalType(const std::string& typeString);

/**
 * Is the type specified a thermal type?
 *
 * @param[in] typeString - a string representation of a PID type.
 * @return true if it's a thermal PID type.
 */
bool isThermalType(const std::string& typeString);

class ThermalController : public PIDController
{
  public:
    static std::unique_ptr<PIDController>
        createThermalPid(ZoneInterface* owner, const std::string& id,
                         const std::vector<std::string>& inputs,
                         double setpoint, const ec::pidinfo& initial,
                         const ThermalType& type);

    ThermalController(const std::string& id,
                      const std::vector<std::string>& inputs,
                      const ThermalType& type, ZoneInterface* owner) :
        PIDController(id, owner),
        _inputs(inputs), type(type)
    {
    }

    double inputProc(void) override;
    double setptProc(void) override;
    void outputProc(double value) override;

  private:
    std::vector<std::string> _inputs;
    ThermalType type;
};
