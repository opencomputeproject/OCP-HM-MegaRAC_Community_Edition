#!/bin/sh

# Below script is used to set the Host Boot Mode and Setup to BIOS Setup 
busctl call xyz.openbmc_project.Ipmi.Host /xyz/openbmc_project/Ipmi xyz.openbmc_project.Ipmi.Server execute yyyaya{sv} 0x0 0x0 0x8 0x2 0x0 0x1 0x0
busctl call xyz.openbmc_project.Ipmi.Host /xyz/openbmc_project/Ipmi xyz.openbmc_project.Ipmi.Server execute yyyaya{sv} 0x0 0x0 0x8 0x3 0x4 0x1 0x1 0x0
busctl call xyz.openbmc_project.Ipmi.Host /xyz/openbmc_project/Ipmi xyz.openbmc_project.Ipmi.Server execute yyyaya{sv} 0x0 0x0 0x8 0x6 0x5 0x80 0x18 0x0 0x0 0x0 0x0
busctl call xyz.openbmc_project.Ipmi.Host /xyz/openbmc_project/Ipmi xyz.openbmc_project.Ipmi.Server execute yyyaya{sv} 0x0 0x0 0x8 0x2 0x0 0x2 0x0
busctl call xyz.openbmc_project.Ipmi.Host /xyz/openbmc_project/Ipmi xyz.openbmc_project.Ipmi.Server execute yyyaya{sv} 0x0 0x0 0x8 0x2 0x0 0x0 0x0
