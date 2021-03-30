#!/usr/bin/env python

# TODO: openbmc/openbmc#2994 remove python 2 support
try:  # python 2
    import gobject
except ImportError:  # python 3
    from gi.repository import GObject as gobject
import dbus
import dbus.service
import dbus.mainloop.glib
import subprocess
import tempfile
import shutil
import tarfile
import os
from obmc.dbuslib.bindings import get_dbus, DbusProperties, DbusObjectManager

DBUS_NAME = 'org.openbmc.control.BmcFlash'
OBJ_NAME = '/org/openbmc/control/flash/bmc'
DOWNLOAD_INTF = 'org.openbmc.managers.Download'

BMC_DBUS_NAME = 'xyz.openbmc_project.State.BMC'
BMC_OBJ_NAME = '/xyz/openbmc_project/state/bmc0'

UPDATE_PATH = '/run/initramfs'


def doExtract(members, files):
    for tarinfo in members:
        if tarinfo.name in files:
            yield tarinfo


def save_fw_env():
    fw_env = "/etc/fw_env.config"
    lines = 0
    files = []
    envcfg = open(fw_env, 'r')
    try:
        for line in envcfg.readlines():
            # ignore lines that are blank or start with #
            if (line.startswith("#")):
                continue
            if (not len(line.strip())):
                continue
            fn = line.partition("\t")[0]
            files.append(fn)
            lines += 1
    finally:
        envcfg.close()
    if (lines < 1 or lines > 2 or (lines == 2 and files[0] != files[1])):
            raise Exception("Error parsing %s\n" % fw_env)
    shutil.copyfile(files[0], os.path.join(UPDATE_PATH, "image-u-boot-env"))


class BmcFlashControl(DbusProperties, DbusObjectManager):
    def __init__(self, bus, name):
        super(BmcFlashControl, self).__init__(
            conn=bus,
            object_path=name)

        self.Set(DBUS_NAME, "status", "Idle")
        self.Set(DBUS_NAME, "filename", "")
        self.Set(DBUS_NAME, "preserve_network_settings", True)
        self.Set(DBUS_NAME, "restore_application_defaults", False)
        self.Set(DBUS_NAME, "update_kernel_and_apps", False)
        self.Set(DBUS_NAME, "clear_persistent_files", False)
        self.Set(DBUS_NAME, "auto_apply", False)

        bus.add_signal_receiver(
            self.download_error_handler, signal_name="DownloadError")
        bus.add_signal_receiver(
            self.download_complete_handler, signal_name="DownloadComplete")

        self.update_process = None
        self.progress_name = None

    @dbus.service.method(
        DBUS_NAME, in_signature='ss', out_signature='')
    def updateViaTftp(self, ip, filename):
        self.Set(DBUS_NAME, "status", "Downloading")
        self.TftpDownload(ip, filename)

    @dbus.service.method(
        DBUS_NAME, in_signature='s', out_signature='')
    def update(self, filename):
        self.Set(DBUS_NAME, "filename", filename)
        self.download_complete_handler(filename, filename)

    @dbus.service.signal(DOWNLOAD_INTF, signature='ss')
    def TftpDownload(self, ip, filename):
        self.Set(DBUS_NAME, "filename", filename)
        pass

    # Signal handler
    def download_error_handler(self, filename):
        if (filename == self.Get(DBUS_NAME, "filename")):
            self.Set(DBUS_NAME, "status", "Download Error")

    def download_complete_handler(self, outfile, filename):
        # do update
        if (filename != self.Get(DBUS_NAME, "filename")):
            return

        print("Download complete. Updating...")

        self.Set(DBUS_NAME, "status", "Download Complete")
        copy_files = {}

        # determine needed files
        if not self.Get(DBUS_NAME, "update_kernel_and_apps"):
            copy_files["image-bmc"] = True
        else:
            copy_files["image-kernel"] = True
            copy_files["image-rofs"] = True

        if self.Get(DBUS_NAME, "restore_application_defaults"):
            copy_files["image-rwfs"] = True

        # make sure files exist in archive
        try:
            tar = tarfile.open(outfile, "r")
            files = {}
            for f in tar.getnames():
                files[f] = True
            tar.close()
            for f in list(copy_files.keys()):
                if f not in files:
                    raise Exception(
                        "ERROR: File not found in update archive: "+f)

        except Exception as e:
            print(str(e))
            self.Set(DBUS_NAME, "status", "Unpack Error")
            return

        try:
            tar = tarfile.open(outfile, "r")
            tar.extractall(UPDATE_PATH, members=doExtract(tar, copy_files))
            tar.close()

            if self.Get(DBUS_NAME, "clear_persistent_files"):
                print("Removing persistent files")
                try:
                    os.unlink(UPDATE_PATH+"/whitelist")
                except OSError as e:
                    if (e.errno == errno.EISDIR):
                        pass
                    elif (e.errno == errno.ENOENT):
                        pass
                    else:
                        raise

                try:
                    wldir = UPDATE_PATH + "/whitelist.d"

                    for file in os.listdir(wldir):
                        os.unlink(os.path.join(wldir, file))
                except OSError as e:
                    if (e.errno == errno.EISDIR):
                        pass
                    else:
                        raise

            if self.Get(DBUS_NAME, "preserve_network_settings"):
                print("Preserving network settings")
                save_fw_env()

        except Exception as e:
            print(str(e))
            self.Set(DBUS_NAME, "status", "Unpack Error")

        self.Verify()

    def Verify(self):
        self.Set(DBUS_NAME, "status", "Checking Image")
        try:
            subprocess.check_call([
                "/run/initramfs/update",
                "--no-flash",
                "--no-save-files",
                "--no-restore-files",
                "--no-clean-saved-files"])

            self.Set(DBUS_NAME, "status", "Image ready to apply.")
            if (self.Get(DBUS_NAME, "auto_apply")):
                self.Apply()
        except Exception:
            self.Set(DBUS_NAME, "auto_apply", False)
            try:
                subprocess.check_output([
                    "/run/initramfs/update",
                    "--no-flash",
                    "--ignore-mount",
                    "--no-save-files",
                    "--no-restore-files",
                    "--no-clean-saved-files"],
                    stderr=subprocess.STDOUT)
                self.Set(
                    DBUS_NAME, "status",
                    "Deferred for mounted filesystem. reboot BMC to apply.")
            except subprocess.CalledProcessError as e:
                self.Set(
                    DBUS_NAME, "status", "Verify error: %s" % e.output)
            except OSError as e:
                self.Set(
                    DBUS_NAME, "status",
                    "Verify error: problem calling update: %s" % e.strerror)

    def Cleanup(self):
        if self.progress_name:
            try:
                os.unlink(self.progress_name)
                self.progress_name = None
            except oserror as e:
                if e.errno == EEXIST:
                    pass
                raise
        self.update_process = None
        self.Set(DBUS_NAME, "status", "Idle")

    @dbus.service.method(
        DBUS_NAME, in_signature='', out_signature='')
    def Abort(self):
        if self.update_process:
            try:
                self.update_process.kill()
            except Exception:
                pass
        for file in os.listdir(UPDATE_PATH):
            if file.startswith('image-'):
                os.unlink(os.path.join(UPDATE_PATH, file))

        self.Cleanup()

    @dbus.service.method(
        DBUS_NAME, in_signature='', out_signature='s')
    def GetUpdateProgress(self):
        msg = ""

        if self.update_process and self.update_process.returncode is None:
            self.update_process.poll()

        if (self.update_process is None):
            pass
        elif (self.update_process.returncode > 0):
            self.Set(DBUS_NAME, "status", "Apply failed")
        elif (self.update_process.returncode is None):
            pass
        else:            # (self.update_process.returncode == 0)
            files = ""
            for file in os.listdir(UPDATE_PATH):
                if file.startswith('image-'):
                    files = files + file
            if files == "":
                msg = "Apply Complete.  Reboot to take effect."
            else:
                msg = "Apply Incomplete, Remaining:" + files
            self.Set(DBUS_NAME, "status", msg)

        msg = self.Get(DBUS_NAME, "status") + "\n"
        if self.progress_name:
            try:
                prog = open(self.progress_name, 'r')
                for line in prog:
                    # strip off initial sets of xxx\r here
                    # ignore crlf at the end
                    # cr will be -1 if no '\r' is found
                    cr = line.rfind("\r", 0, -2)
                    msg = msg + line[cr + 1:]
            except OSError as e:
                if (e.error == EEXIST):
                    pass
                raise
        return msg

    @dbus.service.method(
        DBUS_NAME, in_signature='', out_signature='')
    def Apply(self):
        progress = None
        self.Set(DBUS_NAME, "status", "Writing images to flash")
        try:
            progress = tempfile.NamedTemporaryFile(
                delete=False, prefix="progress.")
            self.progress_name = progress.name
            self.update_process = subprocess.Popen([
                "/run/initramfs/update"],
                stdout=progress.file,
                stderr=subprocess.STDOUT)
        except Exception as e:
            try:
                progress.close()
                os.unlink(progress.name)
                self.progress_name = None
            except Exception:
                pass
            raise

        try:
            progress.close()
        except Exception:
            pass

    @dbus.service.method(
        DBUS_NAME, in_signature='', out_signature='')
    def PrepareForUpdate(self):
        subprocess.call([
            "fw_setenv",
            "openbmconce",
            "copy-files-to-ram copy-base-filesystem-to-ram"])
        # Set the variable twice so that it is written to both environments of
        # the u-boot redundant environment variables since initramfs can only
        # read one of the environments.
        subprocess.call([
            "fw_setenv",
            "openbmconce",
            "copy-files-to-ram copy-base-filesystem-to-ram"])
        self.Set(DBUS_NAME, "status", "Switch to update mode in progress")
        o = bus.get_object(BMC_DBUS_NAME, BMC_OBJ_NAME)
        intf = dbus.Interface(o, "org.freedesktop.DBus.Properties")
        intf.Set(BMC_DBUS_NAME,
                 "RequestedBMCTransition",
                 "xyz.openbmc_project.State.BMC.Transition.Reboot")


if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = get_dbus()
    obj = BmcFlashControl(bus, OBJ_NAME)
    mainloop = gobject.MainLoop()

    obj.unmask_signals()
    name = dbus.service.BusName(DBUS_NAME, bus)

    print("Running Bmc Flash Control")
    mainloop.run()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
