#!/usr/bin/env python

# TODO: openbmc/openbmc#2994 remove python 2 support
try:  # python 2
    import gobject
except ImportError:  # python 3
    from gi.repository import GObject as gobject
import dbus
import dbus.service
import dbus.mainloop.glib
from obmc.dbuslib.bindings import get_dbus, DbusProperties, DbusObjectManager

DBUS_NAME = 'org.openbmc.control.Chassis'
OBJ_NAME = '/org/openbmc/control/chassis0'
CONTROL_INTF = 'org.openbmc.Control'

MACHINE_ID = '/etc/machine-id'

POWER_OFF = 0
POWER_ON = 1

BOOTED = 100


class ChassisControlObject(DbusProperties, DbusObjectManager):
    def getUuid(self):
        uuid = ""
        try:
            with open(MACHINE_ID) as f:
                data = f.readline().rstrip('\n')
                if (len(data) == 32):
                    uuid = data
                else:
                    print("ERROR:  UUID is not formatted correctly: " + data)
        except Exception:
            print("ERROR: Unable to open uuid file: " + MACHINE_ID)

        return uuid

    def __init__(self, bus, name):
        super(ChassisControlObject, self).__init__(
            conn=bus,
            object_path=name)
        # load utilized objects
        self.dbus_objects = {
            'power_control': {
                'bus_name': 'org.openbmc.control.Power',
                'object_name': '/org/openbmc/control/power0',
                'interface_name': 'org.openbmc.control.Power'
            },
            'host_services': {
                'bus_name': 'org.openbmc.HostServices',
                'object_name': '/org/openbmc/HostServices',
                'interface_name': 'org.openbmc.HostServices'
            },
            'systemd': {
                'bus_name': 'org.freedesktop.systemd1',
                'object_name': '/org/freedesktop/systemd1',
                'interface_name': 'org.freedesktop.systemd1.Manager'
            },
        }

        # uuid
        self.Set(DBUS_NAME, "uuid", self.getUuid())
        self.Set(DBUS_NAME, "reboot", 0)

        bus.add_signal_receiver(self.power_button_signal_handler,
                                dbus_interface="org.openbmc.Button",
                                signal_name="Released",
                                path="/org/openbmc/buttons/power0")
        bus.add_signal_receiver(self.long_power_button_signal_handler,
                                dbus_interface="org.openbmc.Button",
                                signal_name="PressedLong",
                                path="/org/openbmc/buttons/power0")
        bus.add_signal_receiver(self.softreset_button_signal_handler,
                                dbus_interface="org.openbmc.Button",
                                signal_name="Released",
                                path="/org/openbmc/buttons/reset0")

        bus.add_signal_receiver(self.host_watchdog_signal_handler,
                                dbus_interface="org.openbmc.Watchdog",
                                signal_name="WatchdogError")


    def getInterface(self, name):
        o = self.dbus_objects[name]
        obj = bus.get_object(o['bus_name'], o['object_name'], introspect=False)
        return dbus.Interface(obj, o['interface_name'])

    @dbus.service.method(DBUS_NAME,
                         in_signature='', out_signature='')
    def powerOn(self):
        print("Turn on power and boot")
        self.Set(DBUS_NAME, "reboot", 0)
        intf = self.getInterface('systemd')
        f = getattr(intf, 'StartUnit')
        f.call_async('obmc-host-start@0.target', 'replace')
        return None

    @dbus.service.method(DBUS_NAME,
                         in_signature='', out_signature='')
    def powerOff(self):
        print("Turn off power")
        intf = self.getInterface('systemd')
        f = getattr(intf, 'StartUnit')
        f.call_async('obmc-chassis-hard-poweroff@0.target', 'replace')
        return None

    @dbus.service.method(DBUS_NAME,
                         in_signature='', out_signature='')
    def softPowerOff(self):
        print("Soft off power")
        intf = self.getInterface('systemd')
        f = getattr(intf, 'StartUnit')
        f.call_async('obmc-host-shutdown@0.target', 'replace')
        return None

    @dbus.service.method(DBUS_NAME,
                         in_signature='', out_signature='')
    def reboot(self):
        print("Rebooting")
        if self.getPowerState() != POWER_OFF:
            self.Set(DBUS_NAME, "reboot", 1)
            self.powerOff()
        return None

    @dbus.service.method(DBUS_NAME,
                         in_signature='', out_signature='')
    def softReboot(self):
        print("Soft Rebooting")
        if self.getPowerState() != POWER_OFF:
            self.Set(DBUS_NAME, "reboot", 1)
            self.softPowerOff()
        return None

    @dbus.service.method(DBUS_NAME,
                         in_signature='', out_signature='')
    def quiesce(self):
        intf = self.getInterface('systemd')
        f = getattr(intf, 'StartUnit')
        f.call_async('obmc-host-quiesce@0.target', 'replace')
        return None

    @dbus.service.method(DBUS_NAME,
                         in_signature='', out_signature='i')
    def getPowerState(self):
        intf = self.getInterface('power_control')
        return intf.getPowerState()

    # Signal handler

    def power_button_signal_handler(self):
        # toggle power, power-on / soft-power-off
        state = self.getPowerState()
        if state == POWER_OFF:
            self.powerOn()
        elif state == POWER_ON:
            self.softPowerOff()

    def long_power_button_signal_handler(self):
        print("Long-press button, hard power off")
        self.powerOff()

    def softreset_button_signal_handler(self):
        self.softReboot()

    def host_watchdog_signal_handler(self):
        print("Watchdog Error, Going to quiesce")
        self.quiesce()


if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = get_dbus()
    obj = ChassisControlObject(bus, OBJ_NAME)
    mainloop = gobject.MainLoop()

    obj.unmask_signals()
    name = dbus.service.BusName(DBUS_NAME, bus)

    print("Running ChassisControlService")
    mainloop.run()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
