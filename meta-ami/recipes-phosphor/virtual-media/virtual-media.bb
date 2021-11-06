SUMMARY = "Virtual Media Service"
DESCRIPTION = "Virtual Media Service"

RDEPENDS_${PN} += "nfs-export-root nbdkit"

SRC_URI = "git://git@github.com/Intel-BMC/provingground.git;protocol=ssh \
           file://0001-Virtual-Media-Using-REDFISH.patch \
           file://0002-Added-Active-KVM-Property.patch \
           file://0003-Added-Active-NBD-Property.patch \
           file://0004-Added-Eject-Porperty.patch \
           file://0005-Fixed-compile-errors-OSP2-2.patch \
           file://0006-Added-VM-Redfish-support-through-NFS.patch \
           file://rmstate \
           file://0007-mediasession-D_bus-OBJ-creation.patch \
           file://bios.sh \
           file://0008-Added-BIOSMode-Method.patch \
           file://0009-Added-VMedia-Eject-Property-OSP2.2.patch \
           file://0010-Added-VM-via-https-support.patch \
           file://0011-removed-VM-polling-on-proxy-mode-in-redfish.patch \
           file://0012-Added-new-dbus-call-to-save-Vmedia-Credentials.patch \
           "


SRCREV = "4aec5d06d6adbaf53dbe7f18ea9f803eb2198b86"

S = "${WORKDIR}/git/virtual-media/"
PV = "1.0+git${SRCPV}"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.VirtualMedia.service"

DEPENDS = "udev boost nlohmann-json systemd sdbusplus boost"

inherit cmake systemd

EXTRA_OECMAKE += "-DYOCTO_DEPENDENCIES=ON"

FULL_OPTIMIZATION = "-Os -pipe -flto"
#FULL_OPTIMIZATION = "-Os -pipe -flto -fno-rtti"

do_install_append() {

        install -d ${D}/etc/vm
        install -m 0644 ${WORKDIR}/rmstate ${D}/etc/vm/
        install -m 0777 ${WORKDIR}/bios.sh ${D}/etc/
}

