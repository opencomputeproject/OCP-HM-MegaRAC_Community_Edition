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

#include "dbusactiveread.hpp"

#include "util.hpp"

#include <chrono>
#include <cmath>
#include <iostream>

ReadReturn DbusActiveRead::read(void)
{
    struct SensorProperties settings;
    double value;

    _helper->getProperties(_bus, _service, _path, &settings);

    value = settings.value * pow(10, settings.scale);

    /*
     * Technically it might not be a value from now, but there's no timestamp
     * on Sensor.Value yet.
     */
    struct ReadReturn r = {value, std::chrono::high_resolution_clock::now()};

    return r;
}
