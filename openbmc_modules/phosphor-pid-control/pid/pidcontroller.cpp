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

#include "pidcontroller.hpp"

#include "ec/pid.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include <vector>

void PIDController::process(void)
{
    double input;
    double setpt;
    double output;

    // Get setpt value
    setpt = setptProc();

    // Get input value
    input = inputProc();

    auto info = getPIDInfo();

    // if no hysteresis, maintain previous behavior
    if (info->positiveHysteresis == 0 && info->negativeHysteresis == 0)
    {
        // Calculate new output
        output = ec::pid(info, input, setpt);

        // this variable isn't actually used in this context, but we're setting
        // it here incase somebody uses it later it's the correct value
        lastInput = input;
    }
    else
    {
        // initialize if not set yet
        if (std::isnan(lastInput))
        {
            lastInput = input;
        }

        // if reading is outside of hysteresis bounds, use it for reading,
        // otherwise use last reading without updating it first
        else if ((input - lastInput) > info->positiveHysteresis)
        {
            lastInput = input;
        }
        else if ((lastInput - input) > info->negativeHysteresis)
        {
            lastInput = input;
        }

        output = ec::pid(info, lastInput, setpt);
    }

    // Output new value
    outputProc(output);

    return;
}
