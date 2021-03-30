#pragma once

#include "controller.hpp"
#include "ec/stepwise.hpp"
#include "fan.hpp"

#include <limits>
#include <memory>
#include <vector>

class ZoneInterface;

class StepwiseController : public Controller
{
  public:
    static std::unique_ptr<Controller>
        createStepwiseController(ZoneInterface* owner, const std::string& id,
                                 const std::vector<std::string>& inputs,
                                 const ec::StepwiseInfo& initial);

    StepwiseController(const std::string& id,
                       const std::vector<std::string>& inputs,
                       ZoneInterface* owner) :
        Controller(),
        _owner(owner), _id(id), _inputs(inputs)
    {
    }

    double inputProc(void) override;

    void outputProc(double value) override;

    void process(void) override;

    std::string getID(void) override
    {
        return _id;
    }

    ec::StepwiseInfo& get_stepwise_info(void)
    {
        return _stepwise_info;
    }

  protected:
    ZoneInterface* _owner;

  private:
    // parameters
    ec::StepwiseInfo _stepwise_info;
    std::string _id;
    std::vector<std::string> _inputs;
    double lastInput = std::numeric_limits<double>::quiet_NaN();
    double lastOutput = std::numeric_limits<double>::quiet_NaN();
};
