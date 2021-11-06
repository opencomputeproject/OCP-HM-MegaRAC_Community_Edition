FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SUMMARY = "Manage firmware upgrade settings"
DESCRIPTION = "Allow user to specify which settings to preserve upon firmware upgrade"

LICENSE = "CLOSED"

inherit autotools pkgconfig
inherit python3native
inherit obmc-phosphor-dbus-service

SRC_URI += "file://src/"

DBUS_SERVICE_${PN} = "xyz.openbmc_project.Software.Preserve.service"

DEPENDS += "systemd"
DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus ${PYTHON_PN}-sdbus++-native"
DEPENDS += "phosphor-logging"

S = "${WORKDIR}/src"
