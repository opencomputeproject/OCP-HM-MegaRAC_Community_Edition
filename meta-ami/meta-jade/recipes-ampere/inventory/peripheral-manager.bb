SUMMARY = "Peripheral Manager"
DESCRIPTION = "Daemon to monitor and report the platform peripheral status"
HOMEPAGE = "https://github.com/ampere-openbmc/ampere-misc"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit meson pkgconfig
inherit systemd

DEPENDS += "sdbusplus"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "sdeventplus"
DEPENDS += "phosphor-logging"
DEPENDS += "nlohmann-json"
DEPENDS += "i2c-tools"

S = "${WORKDIR}/git/peripheral-manager"
SRC_URI = "git://github.com/ampere-openbmc/ampere-misc.git;protocol=https;branch=ampere"
SRC_URI += " file://config.json"
SRCREV = "11c6b42c4afadac64c27e5fe866f6e386ca50206"

SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.peripheral.manager.service"

do_install_append_mtjade() {
    install -d ${D}/etc/peripheral
    install -m 0644 -D ${WORKDIR}/config.json ${D}/etc/peripheral/config.json
}

