#pragma once

#include "sensorset.hpp"

#include <optional>
#include <string>
#include <vector>

/** @class AverageHandling
 *  @brief Handle avergae value when AVERAGE_* is set in env
 */
class Average
{
  public:
    /** @brief The key type of average_set */
    using averageKey = SensorSet::key_type;

    /** @brief <average, average_interval>
     *  average is the value of power*_average.
     *  average_interval is the value of power*_average_interval.
     */
    using averageValue = std::pair<int64_t, int64_t>;

    /** @brief Store sensors' <averageKey, averageValue> map */
    using averageMap = std::map<averageKey, averageValue>;

    /** @brief Get averageValue in averageMap based on averageKey.
     *  This function will be called only when the env AVERAGE_xxx is set to
     *  true.
     *
     *  @param[in] sensorKey - Sensor details
     *
     *  @return - Optional
     *      return {}, if sensorKey can not be found in averageMap
     *      return averageValue, if sensorKey can be found in averageMap
     */
    std::optional<averageValue>
        getAverageValue(const averageKey& sensorKey) const;

    /** @brief Set average value in averageMap based on sensor key.
     *  This function will be called only when the env AVERAGE_xxx is set to
     *  true.
     *
     *  @param[in] sensorKey - Sensor details
     *  @param[in] sensorValue - The related average values of this sensor
     */
    void setAverageValue(const averageKey& sensorKey,
                         const averageValue& sensorValue);

    /** @brief Calculate the average value.
     *
     *  @param[in] preAverage - The previous average value from *_average file
     *  @param[in] preInterval - The previous interval value from
     *                           *_average_interval file
     *  @param[in] curAverage - The current average value from *_average file
     *  @param[in] curInterval - The current interval value from
     *                           *_average_interval file
     *
     *  @return value - Optional
     *      return {}, if curInterval-preInterval=0
     *      return new calculated average value, if curInterval-preInterval>0
     */
    static std::optional<int64_t> calcAverage(int64_t preAverage,
                                              int64_t preInterval,
                                              int64_t curAverage,
                                              int64_t curInterval);

  private:
    /** @brief Store the previous average sensor map */
    averageMap _previousAverageMap;
};