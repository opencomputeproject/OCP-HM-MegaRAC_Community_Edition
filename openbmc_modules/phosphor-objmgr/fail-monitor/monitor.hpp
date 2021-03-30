#pragma once

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace unit
{
namespace failure
{

/**
 * @class Monitor
 *
 * This class will analyze a unit to see if it is in the failed
 * state.  If it is, it will either start or stop a target unit.
 *
 * The use case is for running from the OnFailure directive in a
 * unit file.  If that unit keeps failing and restarting, it will
 * eventually exceed its rate limits and stop being restarted.
 * This application will allow another unit to be started when that
 * occurs.
 */
class Monitor
{
  public:
    /**
     * The valid actions - either starting or stopping a unit
     */
    enum class Action
    {
        start,
        stop
    };

    Monitor() = delete;
    Monitor(const Monitor&) = delete;
    Monitor(Monitor&&) = default;
    Monitor& operator=(const Monitor&) = delete;
    Monitor& operator=(Monitor&&) = default;
    ~Monitor() = default;

    /**
     * Constructor
     *
     * @param[in] sourceUnit - the source unit
     * @param[in] targetUnit - the target unit
     * @param[in] action - the action to run on the target
     */
    Monitor(const std::string& sourceUnit, const std::string& targetUnit,
            Action action) :
        bus(std::move(sdbusplus::bus::new_default())),
        source(sourceUnit), target(targetUnit), action(action)
    {
    }

    /**
     * Analyzes the source unit to check if it is in a failed state.
     * If it is, then it runs the action on the target unit.
     */
    void analyze();

  private:
    /**
     * Returns the dbus object path of the source unit
     */
    std::string getSourceUnitPath();

    /**
     * Says if the unit object passed in has an
     * ActiveState property equal to 'failed'.
     *
     * @param[in] path - the unit object path to check
     *
     * @return - true if this unit is in the failed state
     */
    bool inFailedState(const std::string&& path);

    /**
     * Runs the action on the target unit.
     */
    void runTargetAction();

    /**
     * The dbus object
     */
    sdbusplus::bus::bus bus;

    /**
     * The source unit
     */
    const std::string source;

    /**
     * The target unit
     */
    const std::string target;

    /**
     * The action to run on the target if the source
     * unit is in failed state.
     */
    const Action action;
};
} // namespace failure
} // namespace unit
} // namespace phosphor
