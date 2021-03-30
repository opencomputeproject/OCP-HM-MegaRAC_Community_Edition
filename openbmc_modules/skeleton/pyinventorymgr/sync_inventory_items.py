#!/usr/bin/python -u
#
# Copyright 2016 IBM Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os
import dbus
import uuid
import argparse
import subprocess
import obmc.mapper
import shutil

INV_INTF_NAME = 'xyz.openbmc_project.Inventory.Item.NetworkInterface'
NET_DBUS_NAME = 'org.openbmc.NetworkManager'
NET_OBJ_NAME = '/org/openbmc/NetworkManager/Interface'
CHS_DBUS_NAME = 'org.openbmc.control.Chassis'
CHS_INTF_NAME = 'xyz.openbmc_project.Common.UUID'
CHS_OBJ_NAME = '/org/openbmc/control/chassis0'
PROP_INTF_NAME = 'org.freedesktop.DBus.Properties'
INVENTORY_ROOT = '/xyz/openbmc_project/inventory'
NETWORK_ROOT = '/xyz/openbmc_project/network'
ETHERNET_INTF_NAME = 'xyz.openbmc_project.Network.EthernetInterface'
MAC_INTF_NAME = 'xyz.openbmc_project.Network.MACAddress'

FRUS = {}

# IEEE 802 MAC address mask for locally administered.
# This means the admin has set the MAC and is no longer
# the unique number set by the device manufacturer.
MAC_LOCALLY_ADMIN_MASK = 0x20000000000


# Get inventory MACAddress value.
def get_bmc_mac_address(bus, prop):
    mapper = obmc.mapper.Mapper(bus)

    # Get the inventory subtree, limited
    # to objects that implement NetworkInterface.
    for path, info in \
        mapper.get_subtree(
            path=INVENTORY_ROOT,
            interfaces=[INV_INTF_NAME]).iteritems():

            # Find a NetworkInterface with 'bmc' in the path.
            if 'bmc' not in path:
                continue

            # Only expecting a single service to implement
            # NetworkInterface.  Get the service connection
            # from the mapper response
            conn = info.keys()[0]

            # Get the inventory object implementing NetworkInterface.
            obj = bus.get_object(conn, path)

            # Get the MAC address
            mproxy = obj.get_dbus_method('Get', PROP_INTF_NAME)
            return mproxy(INV_INTF_NAME, prop)


# Get Network Interface object.
def get_network_interface_object(bus):
    mapper = obmc.mapper.Mapper(bus)

    # Get the network subtree, limited
    # to objects that implements EthernetInterface.
    for path, info in \
        mapper.get_subtree(
            path=NETWORK_ROOT,
            interfaces=[ETHERNET_INTF_NAME]).iteritems():

        # Find the one which is having physical interface,it may happen
        # that vlan interface is there and we want the physical
        # interface here.
        if path.split('/')[-1].find('_') < 0:
            service = info.keys()[0]
            net_obj = bus.get_object(service, path)
            return net_obj


# Get inventory UUID value.
def get_uuid(bus, prop):
    mapper = obmc.mapper.Mapper(bus)

    # Get the inventory subtree, limited
    # to objects that implement UUID.
    resp = mapper.get_subtree(
        path=INVENTORY_ROOT,
        interfaces=[CHS_INTF_NAME])

    # Only expecting a single object to implement UUID.
    try:
        path, info = resp.items()[0]
    except IndexError as e:
        return None

    # Only expecting a single service to implement
    # UUID.  Get the service connection
    # from the mapper response
    conn = info.keys()[0]

    # Get the inventory object implementing UUID.
    obj = bus.get_object(conn, path)

    # Get the uuid
    mproxy = obj.get_dbus_method('Get', PROP_INTF_NAME)
    return mproxy(CHS_INTF_NAME, prop)


# Get the value of the mac on the system (from u-boot) without ':' separators
def get_sys_mac(obj):
    sys_mac = ''
    try:
        sys_mac = subprocess.check_output(["fw_printenv", "-n", "ethaddr"])
    except Exception:
        # Handle when mac does not exist in u-boot
        return sys_mac
    return sys_mac


# Replace the value of the system mac with the value of the inventory
# MAC if the system MAC is not locally administered because this means
# the system admin has purposely set the MAC
def sync_mac(obj, inv_mac, sys_mac):
    if sys_mac:
        # Convert sys MAC to int to perform bitwise '&'
        sys_mac = sys_mac.replace(":", "")
        int_sys_mac = int(sys_mac, 16)
    else:
        # Set mac to 0 for when u-boot mac is not present
        int_sys_mac = 0
    if not int_sys_mac & MAC_LOCALLY_ADMIN_MASK:
        # Sys MAC is not locally administered, go replace it with inv value
        intf = dbus.Interface(obj, dbus.PROPERTIES_IFACE)
        intf.Set(MAC_INTF_NAME, "MACAddress", inv_mac)


# Set sys uuid, this reboots the BMC for the value to take effect
def set_sys_uuid(uuid):
    rc = subprocess.call(["fw_setenv", "uuid", uuid])
    if rc == 0:
        print "Rebooting BMC to set uuid"
        # TODO Uncomment once sync from u-boot to /etc/machine-id is in place
        # Issue openbmc/openbmc#479
        # rc = subprocess.call(["reboot"])
    else:
        print "Error setting uuid"


if __name__ == '__main__':
    arg = argparse.ArgumentParser()
    arg.add_argument('-p')
    arg.add_argument('-s')

    opt = arg.parse_args()
    prop_name = opt.p
    sync_type = opt.s

    bus = dbus.SystemBus()
    if sync_type == "mac":
        inv_mac = get_bmc_mac_address(bus, prop_name)
        if not inv_mac:
            sys.exit(1)
        net_obj = get_network_interface_object(bus)
        if not net_obj:
            print "Unable to get the network object"
            sys.exit(1)
        sys_mac = get_sys_mac(net_obj)
        if inv_mac != sys_mac:
            print "Inventory MAC=%s,System MAC=%s" % (inv_mac, sys_mac)
            sync_mac(net_obj, inv_mac, sys_mac)
    elif sync_type == "uuid":
            inv_uuid = get_uuid(bus, prop_name)
            if inv_uuid:
                inv_uuid = uuid.UUID(inv_uuid)
                chs_obj = bus.get_object(CHS_DBUS_NAME, CHS_OBJ_NAME)
                chs_uuid = get_sys_uuid(chs_obj)
                if inv_uuid != sys_uuid:
                    set_sys_uuid(inv_uuid)

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
