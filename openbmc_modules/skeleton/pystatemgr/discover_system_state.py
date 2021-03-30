#!/usr/bin/env python

import dbus


dbus_objects = {
    'power': {
        'bus_name': 'org.openbmc.control.Power',
        'object_name': '/org/openbmc/control/power0',
        'interface_name': 'org.openbmc.control.Power'
    },
    'occstatus0': {
        'bus_name': 'org.openbmc.Sensors',
        'object_name': '/org/openbmc/sensors/host/cpu0/OccStatus',
        'interface_name': 'org.openbmc.SensorValue'
    },
    'occstatus1': {
        'bus_name': 'org.openbmc.Sensors',
        'object_name': '/org/openbmc/sensors/host/cpu1/OccStatus',
        'interface_name': 'org.openbmc.SensorValue'
    },
    'bootprogress': {
        'bus_name': 'org.openbmc.Sensors',
        'object_name': '/org/openbmc/sensors/host/BootProgress',
        'interface_name': 'org.openbmc.SensorValue'
    },
    'host': {
        'bus_name': 'xyz.openbmc_project.State.Host',
        'object_name': '/xyz/openbmc_project/state/host0',
        'interface_name': 'xyz.openbmc_project.State.Host'
    },
    'settings': {
        'bus_name': 'org.openbmc.settings.Host',
        'object_name': '/org/openbmc/settings/host0',
        'interface_name': 'org.freedesktop.DBus.Properties'
    },
    'system': {
        'bus_name': 'org.openbmc.managers.System',
        'object_name': '/org/openbmc/managers/System',
        'interface_name': 'org.freedesktop.DBus.Properties'
    },
    'powersupplyredundancy': {
        'bus_name': 'org.openbmc.Sensors',
        'object_name': '/org/openbmc/sensors/host/PowerSupplyRedundancy',
        'interface_name': 'org.openbmc.SensorValue'
    },
    'turboallowed': {
        'bus_name': 'org.openbmc.Sensors',
        'object_name': '/org/openbmc/sensors/host/TurboAllowed',
        'interface_name': 'org.openbmc.SensorValue'
    },
    'powersupplyderating': {
        'bus_name': 'org.openbmc.Sensors',
        'object_name': '/org/openbmc/sensors/host/PowerSupplyDerating',
        'interface_name': 'org.openbmc.SensorValue'
    },
}


def getInterface(bus, objs, key):
    obj = bus.get_object(
        objs[key]['bus_name'], objs[key]['object_name'], introspect=False)
    return dbus.Interface(obj, objs[key]['interface_name'])


def getProperty(bus, objs, key, prop):
    obj = bus.get_object(
        objs[key]['bus_name'], objs[key]['object_name'], introspect=False)
    intf = dbus.Interface(obj, dbus.PROPERTIES_IFACE)
    return intf.Get(objs[key]['interface_name'], prop)


def setProperty(bus, objs, key, prop, prop_value):
    obj = bus.get_object(
        objs[key]['bus_name'], objs[key]['object_name'])
    intf = dbus.Interface(obj, dbus.PROPERTIES_IFACE)
    return intf.Set(objs[key]['interface_name'], prop, prop_value)


bus = dbus.SystemBus()
pgood = getProperty(bus, dbus_objects, 'power', 'pgood')

if (pgood == 1):
    intf = getInterface(bus, dbus_objects, 'bootprogress')
    intf.setValue("FW Progress, Starting OS")
    intf = getInterface(bus, dbus_objects, 'occstatus0')
    intf.setValue("Enabled")
    intf = getInterface(bus, dbus_objects, 'occstatus1')
    intf.setValue("Enabled")
else:
    # Power is off, so check power policy
    settings_intf = getInterface(bus, dbus_objects, 'settings')
    system_intf = getInterface(bus, dbus_objects, 'system')
    system_last_state = system_intf.Get("org.openbmc.managers.System",
                                        "system_last_state")
    power_policy = settings_intf.Get("org.openbmc.settings.Host",
                                     "power_policy")
    print "Last System State:"+system_last_state+"Power Policy:"+power_policy

    if (power_policy == "ALWAYS_POWER_ON" or
       (power_policy == "RESTORE_LAST_STATE" and
            system_last_state == "HOST_POWERED_ON")):
        setProperty(bus, dbus_objects, 'host', 'RequestedHostTransition',
                    'xyz.openbmc_project.State.Host.Transition.On')

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
