/**
 * Copyright © 2019 IBM Corporation
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
#include "mtms.hpp"

namespace openpower
{
namespace pels
{

MTMS::MTMS()
{
    memset(_machineTypeAndModel.data(), 0, mtmSize);
    memset(_serialNumber.data(), 0, snSize);
}

MTMS::MTMS(const std::string& typeModel, const std::string& serialNumber)
{
    memset(_machineTypeAndModel.data(), 0, mtmSize);
    memset(_serialNumber.data(), 0, snSize);

    // Copy in as much as the fields as possible
    for (size_t i = 0; i < mtmSize; i++)
    {
        if (typeModel.size() > i)
        {
            _machineTypeAndModel[i] = typeModel[i];
        }
    }

    for (size_t i = 0; i < snSize; i++)
    {
        if (serialNumber.size() > i)
        {
            _serialNumber[i] = serialNumber[i];
        }
    }
}

MTMS::MTMS(Stream& stream)
{
    for (size_t i = 0; i < mtmSize; i++)
    {
        stream >> _machineTypeAndModel[i];
    }

    for (size_t i = 0; i < snSize; i++)
    {
        stream >> _serialNumber[i];
    }
}

Stream& operator<<(Stream& s, const MTMS& mtms)
{
    for (size_t i = 0; i < MTMS::mtmSize; i++)
    {
        s << mtms.machineTypeAndModelRaw()[i];
    }

    for (size_t i = 0; i < MTMS::snSize; i++)
    {
        s << mtms.machineSerialNumberRaw()[i];
    }

    return s;
}

Stream& operator>>(Stream& s, MTMS& mtms)
{
    std::array<uint8_t, MTMS::mtmSize> mtm;

    for (size_t i = 0; i < MTMS::mtmSize; i++)
    {
        s >> mtm[i];
    }

    mtms.setMachineTypeAndModel(mtm);

    std::array<uint8_t, MTMS::snSize> sn;
    for (size_t i = 0; i < MTMS::snSize; i++)
    {
        s >> sn[i];
    }

    mtms.setMachineSerialNumber(sn);

    return s;
}
} // namespace pels
} // namespace openpower
