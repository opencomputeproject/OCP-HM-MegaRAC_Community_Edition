#pragma once

#include <memory>
#include <string>

namespace witherspoon
{
namespace power
{

/**
 * @class Device
 *
 * This object is an abstract base class for a device that
 * can be monitored for power faults.
 */
class Device
{
  public:
    Device() = delete;
    virtual ~Device() = default;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = default;
    Device& operator=(Device&&) = default;

    /**
     * Constructor
     *
     * @param name - the device name
     * @param inst - the device instance
     */
    Device(const std::string& name, size_t inst) : name(name), instance(inst)
    {}

    /**
     * Returns the instance number
     */
    inline auto getInstance() const
    {
        return instance;
    }

    /**
     * Returns the name
     */
    inline auto getName() const
    {
        return name;
    }

    /**
     * Pure virtual function to analyze an error
     */
    virtual void analyze() = 0;

    /**
     * Stubbed virtual function to call when it's known
     * the chip is in error state.  Override if functionality
     * is required
     */
    virtual void onFailure()
    {}

    /**
     * Pure virtual function to clear faults on the device
     */
    virtual void clearFaults() = 0;

  private:
    /**
     * the device name
     */
    const std::string name;

    /**
     * the device instance number
     */
    const size_t instance;
};

} // namespace power
} // namespace witherspoon
