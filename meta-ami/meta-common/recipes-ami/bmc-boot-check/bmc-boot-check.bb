DESCRIPTION = "Indentify if bmc boot is caused by AC power loss"
PR = "r1"
PV = "0.1"

LICENSE = "CLOSED"

SRC_URI = "file://bmc-boot-check.sh"

inherit systemd
inherit obmc-phosphor-systemd

DEPENDS = "systemd"
RDEPENDS:${PN} = "bash"

SYSTEMD_PACKAGES = "${PN}"
do_install() {
    install -d ${D}/${bindir}
    install -m 0755 ${WORKDIR}/bmc-boot-check.sh ${D}/${bindir}/
}

SYSTEMD_SERVICE:${PN} = "xyz.openbmc_project.bmcbootcheck.service"
