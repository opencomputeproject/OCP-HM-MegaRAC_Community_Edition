SUMMARY = "Ampere Driver Binder"
DESCRIPTION = "Daemon to Bind/Unbind driver base on the power state and host state"
HOMEPAGE = "https://github.com/ampere-openbmc/ampere-misc"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7dc32e347ce199c5b3f5f131704e7251"

inherit meson pkgconfig
inherit systemd

DEPENDS += "boost"
DEPENDS += "sdbusplus"
DEPENDS += "systemd"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "phosphor-logging"
DEPENDS += "nlohmann-json"
DEPENDS += "gpioplus"

S = "${WORKDIR}/git/driver-binder"

SRC_URI = "git://github.com/ampere-openbmc/ampere-misc.git;protocol=https;branch=ampere"
SRC_URI += " file://driver-binder-config.json"
SRCREV = "4dc7d688419b6a1cf6268e240c798eec920477eb"

SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.AmpDriverBinder.service"

do_install_append_mtjade() {
    install -d ${D}${datadir}/${PN}
    install -m 0644 -D ${WORKDIR}/driver-binder-config.json \
        ${D}${datadir}/${PN}/config.json
}
