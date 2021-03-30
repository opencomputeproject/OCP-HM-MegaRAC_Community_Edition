#!/usr/bin/env python

import os
# TODO: openbmc/openbmc#2994 remove python 2 support
try:  # python 2
    import gobject
except ImportError:  # python 3
    from gi.repository import GObject as gobject
import dbus
import dbus.service
import dbus.mainloop.glib
import subprocess
from obmc.dbuslib.bindings import get_dbus


FLASH_DOWNLOAD_PATH = '/tmp'
DBUS_NAME = 'org.openbmc.managers.Download'
OBJ_NAME = '/org/openbmc/managers/Download'
TFTP_PORT = 69


class DownloadManagerObject(dbus.service.Object):
    def __init__(self, bus, name):
        dbus.service.Object.__init__(self, bus, name)
        bus.add_signal_receiver(
            self.DownloadHandler,
            dbus_interface="org.openbmc.Flash",
            signal_name="Download",
            path_keyword="path")
        bus.add_signal_receiver(
            self.TftpDownloadHandler,
            signal_name="TftpDownload",
            path_keyword="path")

    @dbus.service.signal(DBUS_NAME, signature='ss')
    def DownloadComplete(self, outfile, filename):
        print("Download Complete: "+outfile)
        return outfile

    @dbus.service.signal(DBUS_NAME, signature='s')
    def DownloadError(self, filename):
        pass

    def TftpDownloadHandler(self, ip, filename, path=None):
        try:
            filename = str(filename)
            print("Downloading: "+filename+" from "+ip)
            outfile = FLASH_DOWNLOAD_PATH+"/"+os.path.basename(filename)
            rc = subprocess.call(
                ["tftp", "-l", outfile, "-r", filename, "-g", ip])
            if (rc == 0):
                self.DownloadComplete(outfile, filename)
            else:
                self.DownloadError(filename)

        except Exception as e:
            print("ERROR DownloadManager: "+str(e))
            self.DownloadError(filename)

    # TODO: this needs to be deprecated.
    # Shouldn't call flash interface from here
    def DownloadHandler(self, url, filename, path=None):
        try:
            filename = str(filename)
            print("Downloading: "+filename+" from "+url)
            outfile = FLASH_DOWNLOAD_PATH+"/"+os.path.basename(filename)
            subprocess.call(
                ["tftp", "-l", outfile, "-r", filename, "-g", url])
            obj = bus.get_object("org.openbmc.control.Flash", path)
            intf = dbus.Interface(obj, "org.openbmc.Flash")
            intf.update(outfile)

        except Exception as e:
            print("ERROR DownloadManager: "+str(e))
            obj = bus.get_object("org.openbmc.control.Flash", path)
            intf = dbus.Interface(obj, "org.openbmc.Flash")
            intf.error("Download Error: "+filename)


if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = get_dbus()
    obj = DownloadManagerObject(bus, OBJ_NAME)
    mainloop = gobject.MainLoop()
    name = dbus.service.BusName(DBUS_NAME, bus)

    print("Running Download Manager")
    mainloop.run()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
