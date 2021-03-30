/*
// Copyright (c) 2019 intel Corporation
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

static constexpr char const* coprocessorSubClass = "Coprocessor";
static constexpr char const* otherClass = "Other";

static boost::container::flat_map<int, std::string> pciDeviceClasses{
    {0x00, "UnclassifiedDevice"},
    {0x01, "MassStorageController"},
    {0x02, "NetworkController"},
    {0x03, "DisplayController"},
    {0x04, "MultimediaController"},
    {0x05, "MemoryController"},
    {0x06, "Bridge"},
    {0x07, "CommunicationController"},
    {0x08, "GenericSystemPeripheral"},
    {0x09, "InputDeviceController"},
    {0x0a, "DockingStation"},
    {0x0b, "Processor"},
    {0x0c, "SerialBusController"},
    {0x0d, "WirelessController"},
    {0x0e, "IntelligentController"},
    {0x0f, "SatelliteCommunicationsController"},
    {0x10, "EncryptionController"},
    {0x11, "SignalProcessingController"},
    {0x12, "ProcessingAccelerators"},
    {0x13, "NonEssentialInstrumentation"},
    {0xff, "UnassignedClass"},
};
