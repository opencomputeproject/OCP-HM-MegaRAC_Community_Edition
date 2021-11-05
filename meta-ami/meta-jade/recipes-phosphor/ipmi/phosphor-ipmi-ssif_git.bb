SUMMARY = "Phosphor OpenBMC SSIF to DBUS"
DESCRIPTION = "Phosphor OpenBMC SSIF to DBUS."
PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig
inherit systemd

SYSTEMD_SERVICE_${PN} = "ssifbridge.service"

PROVIDES += "virtual/obmc-host-ipmi-hw"
RPROVIDES_${PN} += "virtual-obmc-host-ipmi-hw"
RRECOMMENDS_${PN} += "phosphor-ipmi-host"

DEPENDS += "systemd"
DEPENDS += "phosphor-logging"
DEPENDS += "sdbusplus"
DEPENDS += "cli11"

S = "${WORKDIR}/git"

FILESEXTRAPATHS_append_mtjade := "${THISDIR}/${PN}:"

SRC_URI = "git://github.com/ampere-openbmc/ssifbridge.git;protocol=https;branch=ampere"
SRC_URI += " file://ssifbridge.service"

SRCREV= "24c0abc6b8dd768281e82c1e168491fdefc9e6df"

do_install_append_mtjade() {
    cp ${WORKDIR}/ssifbridge.service ${D}${systemd_system_unitdir}/ssifbridge.service
}
