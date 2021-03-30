/**
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "drive.hpp"

#include "interfaces.hpp"
#include "sensors/pluggable.hpp"
#include "sysfs/sysfsread.hpp"
#include "sysfs/sysfswrite.hpp"

#include <iostream>
#include <memory>
#include <tuple>

using tstamp = std::chrono::high_resolution_clock::time_point;

#define DRIVE_TIME 1
#define DRIVE_GOAL 2
#define DRIVE DRIVE_TIME
#define MAX_PWM 255

static std::unique_ptr<Sensor> Create(std::string readpath,
                                      std::string writepath)
{
    return std::make_unique<PluggableSensor>(
        readpath, 0, /* default the timeout to disabled */
        std::make_unique<SysFsRead>(readpath),
        std::make_unique<SysFsWrite>(writepath, 0, MAX_PWM));
}

int64_t getAverage(std::tuple<tstamp, int64_t, int64_t>& values)
{
    return (std::get<1>(values) + std::get<2>(values)) / 2;
}

bool valueClose(int64_t value, int64_t goal)
{
#if 0
    int64_t delta = 100; /* within 100 */
    if (value < (goal + delta) &&
        value > (goal - delta))
    {
        return true;
    }
#endif

    /* let's make sure it's below goal. */
    if (value < goal)
    {
        return true;
    }

    return false;
}

static void driveGoal(int64_t& seriesCnt, int64_t setPwm, int64_t goal,
                      std::vector<std::tuple<tstamp, int64_t, int64_t>>& series,
                      std::vector<std::unique_ptr<Sensor>>& fanSensors)
{
    bool reading = true;

    auto& fan0 = fanSensors.at(0);
    auto& fan1 = fanSensors.at(1);

    fan0->write(setPwm);
    fan1->write(setPwm);

    while (reading)
    {
        bool check;
        ReadReturn r0 = fan0->read();
        ReadReturn r1 = fan1->read();
        int64_t n0 = static_cast<int64_t>(r0.value);
        int64_t n1 = static_cast<int64_t>(r1.value);

        tstamp t1 = std::chrono::high_resolution_clock::now();

        series.push_back(std::make_tuple(t1, n0, n1));
        seriesCnt += 1;

        int64_t avgn = (n0 + n1) / 2;
        /* check last three values against goal if this is close */
        check = valueClose(avgn, goal);

        /* We know the last entry is within range. */
        if (check && seriesCnt > 3)
        {
            /* n-2 values */
            std::tuple<tstamp, int64_t, int64_t> nm2 = series.at(seriesCnt - 3);
            /* n-1 values */
            std::tuple<tstamp, int64_t, int64_t> nm1 = series.at(seriesCnt - 2);

            int64_t avgnm2 = getAverage(nm2);
            int64_t avgnm1 = getAverage(nm1);

            int64_t together = (avgnm2 + avgnm1) / 2;

            reading = !valueClose(together, goal);

            if (!reading)
            {
                std::cerr << "finished reaching goal\n";
            }
        }

        /* Early abort for testing. */
        if (seriesCnt > 150000)
        {
            std::cerr << "aborting after 150000 reads.\n";
            reading = false;
        }
    }

    return;
}

static void driveTime(int64_t& seriesCnt, int64_t setPwm, int64_t goal,
                      std::vector<std::tuple<tstamp, int64_t, int64_t>>& series,
                      std::vector<std::unique_ptr<Sensor>>& fanSensors)
{
    using namespace std::literals::chrono_literals;

    bool reading = true;

    auto& fan0 = fanSensors.at(0);
    auto& fan1 = fanSensors.at(1);

    auto& s0 = series.at(0);
    tstamp t0 = std::get<0>(s0);

    fan0->write(setPwm);
    fan1->write(setPwm);

    while (reading)
    {
        ReadReturn r0 = fan0->read();
        ReadReturn r1 = fan1->read();
        int64_t n0 = static_cast<int64_t>(r0.value);
        int64_t n1 = static_cast<int64_t>(r1.value);
        tstamp t1 = std::chrono::high_resolution_clock::now();

        series.push_back(std::make_tuple(t1, n0, n1));

        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0)
                .count();
        if (duration >= (20000000us).count())
        {
            reading = false;
        }
    }

    return;
}

int driveMain(void)
{
    /* Time series of the data, the timestamp after both are read and the
     * values. */
    std::vector<std::tuple<tstamp, int64_t, int64_t>> series;
    int64_t seriesCnt = 0; /* in case vector count isn't constant time */
    int drive = DRIVE;

    /*
     * The fan map:
     *  --> 0 | 4
     *  --> 1 | 5
     *  --> 2 | 6
     *  --> 3 | 7
     */
    std::vector<std::string> fans = {"/sys/class/hwmon/hwmon0/fan0_input",
                                     "/sys/class/hwmon/hwmon0/fan4_input"};

    std::vector<std::string> pwms = {"/sys/class/hwmon/hwmon0/pwm0",
                                     "/sys/class/hwmon/hwmon0/pwm4"};

    std::vector<std::unique_ptr<Sensor>> fanSensors;

    auto fan0 = Create(fans[0], pwms[0]);
    auto fan1 = Create(fans[1], pwms[1]);

    ReadReturn r0 = fan0->read();
    ReadReturn r1 = fan1->read();
    int64_t pwm0_value = static_cast<int64_t>(r0.value);
    int64_t pwm1_value = static_cast<int64_t>(r1.value);

    if (MAX_PWM != pwm0_value || MAX_PWM != pwm1_value)
    {
        std::cerr << "bad PWM starting point.\n";
        return -EINVAL;
    }

    r0 = fan0->read();
    r1 = fan1->read();
    int64_t fan0_start = r0.value;
    int64_t fan1_start = r1.value;
    tstamp t1 = std::chrono::high_resolution_clock::now();

    /*
     * I've done experiments, and seen 9080,10243 as a starting point
     * which leads to a 50% goal of 4830.5, which is higher than the
     * average that they reach, 4668.  -- i guess i could try to figure out
     * a good increase from one to the other, but how fast they're going
     * actually influences how much they influence, so at slower speeds the
     * improvement is less.
     */

    series.push_back(std::make_tuple(t1, fan0_start, fan1_start));
    seriesCnt += 1;

    int64_t average = (fan0_start + fan1_start) / 2;
    int64_t goal = 0.5 * average;

    std::cerr << "goal: " << goal << "\n";

    // fan0 @ 128: 4691
    // fan4 @ 128: 4707

    fanSensors.push_back(std::move(fan0));
    fanSensors.push_back(std::move(fan1));

    if (DRIVE_TIME == drive)
    {
        driveTime(seriesCnt, 128, goal, series, fanSensors);
    }
    else if (DRIVE_GOAL == drive)
    {
        driveGoal(seriesCnt, 128, goal, series, fanSensors);
    }
    tstamp tp = t1;

    /* Output the values and the timepoints as a time series for review. */
    for (const auto& t : series)
    {
        tstamp ts = std::get<0>(t);
        int64_t n0 = std::get<1>(t);
        int64_t n1 = std::get<2>(t);

        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(ts - tp)
                .count();
        std::cout << duration << "us, " << n0 << ", " << n1 << "\n";

        tp = ts;
    }

    return 0;
}
