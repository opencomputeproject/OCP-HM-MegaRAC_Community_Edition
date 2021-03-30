#!/usr/bin/env python

import sys
import subprocess
import dbus
import string
import os
import fcntl
import glib
import gobject
import dbus.service
import dbus.mainloop.glib

DBUS_NAME = 'org.openbmc.HostIpmi'
OBJ_NAME = '/org/openbmc/HostIpmi/1'


def header(seq, netfn, lun, cmd):
    return (
        'seq:   0x%02x\nnetfn: 0x%02x\n\nlun: 0x%02d\ncmd:   0x%02x\n') % (
            seq, netfn, lun, cmd)


def print_request(seq, netfn, lun, cmd, data):
    str = header(seq, netfn, lun, cmd)
    str += 'data:  [%s]' % ', '.join(['0x%02x' % x for x in data])
    print str


def print_response(seq, netfn, lun, cmd, cc, data):
    str = header(seq, netfn, lun, cmd)
    str += 'cc:    0x%02x\ndata:  [%s]' % (
                cc, ', '.join(['0x%02x' % x for x in data])
            )
    print str


class IpmiDebug(dbus.service.Object):
    def __init__(self, bus, name):
        dbus.service.Object.__init__(self, bus, name)

    @dbus.service.signal(DBUS_NAME, "yyyyay")
    def ReceivedMessage(self, seq, netfn, lun, cmd, data):
        print "IPMI packet from host:"
        print_request(seq, netfn, lun, cmd, data)

    @dbus.service.method(DBUS_NAME, "yyyyyay", "x")
    def sendMessage(self, seq, netfn, lun, cmd, ccode, data):
        print "IPMI packet sent to host:"
        print_response(seq, netfn, lun, cmd, ccode, data)
        return 0

    @dbus.service.method(DBUS_NAME)
    def setAttention(self):
        print "IPMI SMS_ATN set"


class ConsoleReader(object):
    def __init__(self, ipmi_obj):
        self.buffer = ''
        self.seq = 0
        self.ipmi_obj = ipmi_obj
        flags = fcntl.fcntl(sys.stdin.fileno(), fcntl.F_GETFL)
        flags |= os.O_NONBLOCK
        fcntl.fcntl(sys.stdin.fileno(), fcntl.F_SETFL, flags)
        glib.io_add_watch(sys.stdin, glib.IO_IN, self.io_callback)

    def io_callback(self, fd, condition):
        chunk = fd.read()
        for char in chunk:
            self.buffer += char
            if char == '\n':
                self.line(self.buffer)
                self.buffer = ''

        return True

    def line(self, data):
        s = data.split(' ')
        if len(s) < 2:
            print "Not enough bytes to form a valid IPMI packet"
            return
        try:
            data = [int(c, 16) for c in s]
        except ValueError:
            return
        self.seq += 1
        self.ipmi_obj.ReceivedMessage(self.seq, data[0], 0, data[1], data[2:])


def main():
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()
    obj = IpmiDebug(bus, OBJ_NAME)
    mainloop = gobject.MainLoop()
    r = ConsoleReader(obj)

    obj.unmask_signals()
    name = dbus.service.BusName(DBUS_NAME, bus)

    print ("Enter IPMI packet as hex values. First three bytes will be used"
           "as netfn and cmd.\nlun will be zero.")
    mainloop.run()


if __name__ == '__main__':
    sys.exit(main())

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
