#!/usr/bin/env python

import os
import sys
# TODO: openbmc/openbmc#2994 remove python 2 support
try:  # python 2
    import gobject
except ImportError:  # python 3
    from gi.repository import GObject as gobject
import dbus
import dbus.service
import dbus.mainloop.glib
import obmc.dbuslib.propertycacher as PropertyCacher
from obmc.dbuslib.bindings import get_dbus, DbusProperties, DbusObjectManager


INTF_NAME = 'org.openbmc.InventoryItem'
DBUS_NAME = 'org.openbmc.Inventory'

FRUS = {}


class Inventory(DbusProperties, DbusObjectManager):
    def __init__(self, bus, name):
        super(Inventory, self).__init__(
            conn=bus,
            object_path=name)


class InventoryItem(DbusProperties):
    def __init__(self, bus, name, data):
        super(InventoryItem, self).__init__(
            conn=bus,
            object_path=name)

        self.name = name

        if 'present' not in data:
            data['present'] = 'False'
        if 'fault' not in data:
            data['fault'] = 'False'
        if 'version' not in data:
            data['version'] = ''

        self.SetMultiple(INTF_NAME, data)

        # this will load properties from cache
        PropertyCacher.load(name, INTF_NAME, self.properties)

    @dbus.service.method(
        INTF_NAME, in_signature='a{sv}', out_signature='')
    def update(self, data):
        self.SetMultiple(INTF_NAME, data)
        PropertyCacher.save(self.name, INTF_NAME, self.properties)

    @dbus.service.method(
        INTF_NAME, in_signature='s', out_signature='')
    def setPresent(self, present):
        self.Set(INTF_NAME, 'present', present)
        PropertyCacher.save(self.name, INTF_NAME, self.properties)

    @dbus.service.method(
        INTF_NAME, in_signature='s', out_signature='')
    def setFault(self, fault):
        self.Set(INTF_NAME, 'fault', fault)
        PropertyCacher.save(self.name, INTF_NAME, self.properties)


def getVersion():
    version = "Error"
    with open('/etc/os-release', 'r') as f:
        for line in f:
            p = line.rstrip('\n')
            parts = line.rstrip('\n').split('=')
            if (parts[0] == "VERSION_ID"):
                version = parts[1]
                version = version.strip('"')
    return version


if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = get_dbus()
    mainloop = gobject.MainLoop()
    obj_parent = Inventory(bus, '/org/openbmc/inventory')
    INVENTORY_FILE = os.path.join(sys.prefix, 'share',
                                  'inventory', 'inventory.json')

    if os.path.exists(INVENTORY_FILE):
        import json
        with open(INVENTORY_FILE, 'r') as f:
            try:
                inv = json.load(f)
            except ValueError:
                print("Invalid JSON detected in " + INVENTORY_FILE)
            else:
                FRUS = inv
    else:
        try:
            import obmc_system_config as System
            FRUS = System.FRU_INSTANCES
        except ImportError:
            pass

    for f in list(FRUS.keys()):
        import obmc.inventory
        obj_path = f.replace("<inventory_root>", obmc.inventory.INVENTORY_ROOT)
        obj = InventoryItem(bus, obj_path, FRUS[f])
        obj_parent.add(obj_path, obj)

        # TODO:  this is a hack to update bmc inventory item with version
        # should be done by flash object
        if (FRUS[f]['fru_type'] == "BMC"):
            version = getVersion()
            obj.update({'version': version})

    obj_parent.unmask_signals()
    name = dbus.service.BusName(DBUS_NAME, bus)
    print("Running Inventory Manager")
    mainloop.run()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
