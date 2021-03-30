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
#include "fru_identity.hpp"

#include "pel_values.hpp"

#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;

namespace openpower
{
namespace pels
{
namespace src
{

namespace
{

/**
 * @brief Fills in the std::array from the string value
 *
 * If the string is shorter than the array, it will be padded with
 * '\0's.
 *
 * @param[in] source - The string to fill in the array with
 * @param[out] target - The input object that supports [] and size()
 */
template <typename T>
void fillArray(const std::string& source, T& target)
{
    for (size_t i = 0; i < target.size(); i++)
    {
        target[i] = (source.size() > i) ? source[i] : '\0';
    }
}

}; // namespace

FRUIdentity::FRUIdentity(Stream& pel)
{
    pel >> _type >> _size >> _flags;

    if (hasPN() || hasMP())
    {
        pel.read(_pnOrProcedureID.data(), _pnOrProcedureID.size());
    }

    if (hasCCIN())
    {
        pel.read(_ccin.data(), _ccin.size());
    }

    if (hasSN())
    {
        pel.read(_sn.data(), _sn.size());
    }
}

size_t FRUIdentity::flattenedSize() const
{
    size_t size = sizeof(_type) + sizeof(_size) + sizeof(_flags);

    if (hasPN() || hasMP())
    {
        size += _pnOrProcedureID.size();
    }

    if (hasCCIN())
    {
        size += _ccin.size();
    }

    if (hasSN())
    {
        size += _sn.size();
    }

    return size;
}

FRUIdentity::FRUIdentity(const std::string& partNumber, const std::string& ccin,
                         const std::string& serialNumber)
{
    _type = substructureType;
    _flags = hardwareFRU;

    setPartNumber(partNumber);
    setCCIN(ccin);
    setSerialNumber(serialNumber);

    _size = flattenedSize();
}

FRUIdentity::FRUIdentity(const std::string& procedureFromRegistry)
{
    _type = substructureType;
    _flags = maintenanceProc;

    setMaintenanceProcedure(procedureFromRegistry);

    _size = flattenedSize();
}

FRUIdentity::FRUIdentity(const std::string& symbolicFRUFromRegistry,
                         bool trustedLocationCode)
{
    _type = substructureType;
    _flags = (trustedLocationCode) ? symbolicFRUTrustedLocCode : symbolicFRU;

    setSymbolicFRU(symbolicFRUFromRegistry);

    _size = flattenedSize();
}

std::optional<std::string> FRUIdentity::getPN() const
{
    if (hasPN())
    {
        // NULL terminated
        std::string pn{_pnOrProcedureID.data()};
        return pn;
    }

    return std::nullopt;
}

std::optional<std::string> FRUIdentity::getMaintProc() const
{
    if (hasMP())
    {
        // NULL terminated
        std::string mp{_pnOrProcedureID.data()};
        return mp;
    }

    return std::nullopt;
}

std::optional<std::string> FRUIdentity::getCCIN() const
{
    if (hasCCIN())
    {
        std::string ccin{_ccin.begin(), _ccin.begin() + _ccin.size()};

        // Don't leave any NULLs in the string (not there usually)
        if (auto pos = ccin.find('\0'); pos != std::string::npos)
        {
            ccin.resize(pos);
        }
        return ccin;
    }

    return std::nullopt;
}

std::optional<std::string> FRUIdentity::getSN() const
{
    if (hasSN())
    {
        std::string sn{_sn.begin(), _sn.begin() + _sn.size()};

        // Don't leave any NULLs in the string (not there usually)
        if (auto pos = sn.find('\0'); pos != std::string::npos)
        {
            sn.resize(pos);
        }
        return sn;
    }

    return std::nullopt;
}

void FRUIdentity::flatten(Stream& pel) const
{
    pel << _type << _size << _flags;

    if (hasPN() || hasMP())
    {
        pel.write(_pnOrProcedureID.data(), _pnOrProcedureID.size());
    }

    if (hasCCIN())
    {
        pel.write(_ccin.data(), _ccin.size());
    }

    if (hasSN())
    {
        pel.write(_sn.data(), _sn.size());
    }
}

void FRUIdentity::setPartNumber(const std::string& partNumber)
{
    _flags |= pnSupplied;
    _flags &= ~maintProcSupplied;

    auto pn = partNumber;

    // Strip leading whitespace on this one.
    while (' ' == pn.front())
    {
        pn = pn.substr(1);
    }

    fillArray(pn, _pnOrProcedureID);

    // ensure null terminated
    _pnOrProcedureID.back() = 0;
}

void FRUIdentity::setCCIN(const std::string& ccin)
{
    _flags |= ccinSupplied;

    fillArray(ccin, _ccin);
}

void FRUIdentity::setSerialNumber(const std::string& serialNumber)
{
    _flags |= snSupplied;

    fillArray(serialNumber, _sn);
}

void FRUIdentity::setMaintenanceProcedure(
    const std::string& procedureFromRegistry)
{
    _flags |= maintProcSupplied;
    _flags &= ~pnSupplied;

    if (pel_values::maintenanceProcedures.count(procedureFromRegistry))
    {
        fillArray(pel_values::maintenanceProcedures.at(procedureFromRegistry),
                  _pnOrProcedureID);
    }
    else
    {
        log<level::ERR>("Invalid maintenance procedure",
                        entry("PROCEDURE=%s", procedureFromRegistry.c_str()));
        strncpy(_pnOrProcedureID.data(), "INVALID", _pnOrProcedureID.size());
    }

    // ensure null terminated
    _pnOrProcedureID.back() = 0;
}

void FRUIdentity::setSymbolicFRU(const std::string& symbolicFRUFromRegistry)
{

    // Treat this has a HW callout.
    _flags |= pnSupplied;
    _flags &= ~maintProcSupplied;

    if (pel_values::symbolicFRUs.count(symbolicFRUFromRegistry))
    {
        fillArray(pel_values::symbolicFRUs.at(symbolicFRUFromRegistry),
                  _pnOrProcedureID);
    }
    else
    {
        log<level::ERR>("Invalid symbolic FRU",
                        entry("FRU=%s", symbolicFRUFromRegistry.c_str()));
        strncpy(_pnOrProcedureID.data(), "INVALID", _pnOrProcedureID.size());
    }

    // ensure null terminated
    _pnOrProcedureID.back() = 0;
}

} // namespace src
} // namespace pels
} // namespace openpower
