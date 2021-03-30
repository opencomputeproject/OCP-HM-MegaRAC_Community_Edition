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

#include "sysfswrite.hpp"

#include <fstream>
#include <iostream>

void SysFsWritePercent::write(double value)
{
    double minimum = getMin();
    double maximum = getMax();

    double range = maximum - minimum;
    double offset = range * value;
    double ovalue = offset + minimum;

    std::ofstream ofs;
    ofs.open(_writePath);
    ofs << static_cast<int64_t>(ovalue);
    ofs.close();

    return;
}

void SysFsWrite::write(double value)
{
    std::ofstream ofs;
    ofs.open(_writePath);
    ofs << static_cast<int64_t>(value);
    ofs.close();

    return;
}
