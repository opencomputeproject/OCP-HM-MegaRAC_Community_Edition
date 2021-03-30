#pragma once

#include <systemd/sd-bus.h>

#include <array>
#include <map>
#include <string>
#include <vector>

struct IPMIFruData
{
    std::string section;
    std::string property;
    std::string delimiter;
};

using DbusProperty = std::string;
using DbusPropertyVec = std::vector<std::pair<DbusProperty, IPMIFruData>>;

using DbusInterface = std::string;
using DbusInterfaceVec = std::vector<std::pair<DbusInterface, DbusPropertyVec>>;

using FruInstancePath = std::string;

struct FruInstance
{
    uint8_t entityID;
    uint8_t entityInstance;
    FruInstancePath path;
    DbusInterfaceVec interfaces;
};

using FruInstanceVec = std::vector<FruInstance>;

using FruId = uint32_t;
using FruMap = std::map<FruId, FruInstanceVec>;
