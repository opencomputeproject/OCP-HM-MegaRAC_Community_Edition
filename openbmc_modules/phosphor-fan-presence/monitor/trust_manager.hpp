#pragma once

#include "tach_sensor.hpp"
#include "trust_group.hpp"
#include "types.hpp"

#include <memory>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace trust
{

/**
 * @class Manager
 *
 * The sensor trust manager class.  It can be asked if a tach sensor's
 * reading can be trusted or not, based on the trust groups the sensor
 * is in.
 *
 * When it finds a group's trust status changing, it will either stop or
 * start the tach error timers for the group's sensors accordingly.
 *
 * See the trust::Group documentation for more details on sensor trust.
 */
class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = default;
    Manager& operator=(Manager&&) = default;
    ~Manager() = default;

    /**
     * Constructor
     *
     * @param[in] functions - trust group creation function vector
     */
    explicit Manager(const std::vector<monitor::CreateGroupFunction>& functions)
    {
        for (auto& create : functions)
        {
            groups.emplace_back(create());
        }
    }

    /**
     * Says if trust groups have been created and
     * need to be checked.
     *
     * @return bool - If there are any trust groups
     */
    inline bool active() const
    {
        return !groups.empty();
    }

    /**
     * Checks if a sensor value can be trusted
     *
     * Checks if the sensor is trusted in each group
     * it belongs to.  Only considered trusted if it is
     * trusted in all groups it belongs to.
     *
     * While checking group trust, the code will also check
     * if the trust status has just changed.  If the status
     * just changed to false, it will stop the tach error
     * timers for that group so these untrusted sensors won't
     * cause errors.  If changed to true, it will start those timers
     * back up again.
     *
     * Note this means groups should be designed such that
     * in the same call to this function a sensor shouldn't
     * make one group change to trusted and another to untrusted.
     *
     * @param[in] sensor - the sensor to check
     *
     * @return bool - if sensor is trusted in all groups or not
     */
    bool checkTrust(const monitor::TachSensor& sensor)
    {
        auto trusted = true;

        for (auto& group : groups)
        {
            if (group->inGroup(sensor))
            {
                bool trust, changed;
                std::tie(trust, changed) = group->checkTrust(sensor);

                if (!trust)
                {
                    trusted = false;

                    if (changed)
                    {
                        group->stopTimers();
                    }
                }
                else
                {
                    if (changed)
                    {
                        group->startTimers();
                    }
                }
            }
        }

        return trusted;
    }

    /**
     * Registers a sensor with any trust groups that are interested
     *
     * @param[in] sensor - the sensor to register
     */
    void registerSensor(std::shared_ptr<monitor::TachSensor>& sensor)
    {
        std::for_each(groups.begin(), groups.end(), [&sensor](auto& group) {
            group->registerSensor(sensor);
        });
    }

  private:
    /**
     * The list of sensor trust groups
     */
    std::vector<std::unique_ptr<Group>> groups;
};

} // namespace trust
} // namespace fan
} // namespace phosphor
