/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once
#include <boost/container/flat_map.hpp>

namespace devices
{

struct CmpStr
{
    bool operator()(const char* a, const char* b) const
    {
        return std::strcmp(a, b) < 0;
    }
};

struct ExportTemplate
{
    ExportTemplate(const char* params, const char* dev) :
        parameters(params), device(dev){};
    const char* parameters;
    const char* device;
};

const boost::container::flat_map<const char*, ExportTemplate, CmpStr>
    exportTemplates{
        {{"24C02", ExportTemplate("24c02 $Address",
                                  "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"24C64", ExportTemplate("24c64 $Address",
                                  "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"ADM1272",
          ExportTemplate("adm1272 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"EEPROM", ExportTemplate("eeprom $Address",
                                   "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"EMC1413",
          ExportTemplate("emc1413 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"Gpio", ExportTemplate("$Index", "/sys/class/gpio/export")},
         {"INA230", ExportTemplate("ina230 $Address",
                                   "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"ISL68137",
          ExportTemplate("isl68137 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"MAX16601",
          ExportTemplate("max16601 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"MAX20730",
          ExportTemplate("max20730 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"MAX20734",
          ExportTemplate("max20734 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"MAX20796",
          ExportTemplate("max20796 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"MAX31725",
          ExportTemplate("max31725 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"MAX31730",
          ExportTemplate("max31730 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"MAX34451",
          ExportTemplate("max34451 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"MAX6654",
          ExportTemplate("max6654 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"PCA9543Mux",
          ExportTemplate("pca9543 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"PCA9544Mux",
          ExportTemplate("pca9544 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"PCA9545Mux",
          ExportTemplate("pca9545 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"PCA9546Mux",
          ExportTemplate("pca9546 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"PCA9547Mux",
          ExportTemplate("pca9547 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"SBTSI", ExportTemplate("sbtsi $Address",
                                  "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"pmbus", ExportTemplate("pmbus $Address",
                                  "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"TMP112", ExportTemplate("tmp112 $Address",
                                   "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"TMP175", ExportTemplate("tmp175 $Address",
                                   "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"TMP421", ExportTemplate("tmp421 $Address",
                                   "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"TMP441", ExportTemplate("tmp441 $Address",
                                   "/sys/bus/i2c/devices/i2c-$Bus/new_device")},
         {"TMP75",
          ExportTemplate("tmp75 $Address",
                         "/sys/bus/i2c/devices/i2c-$Bus/new_device")}}};
} // namespace devices
