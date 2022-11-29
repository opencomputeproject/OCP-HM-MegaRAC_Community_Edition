SUMMARY = "VC BMC Ready Pin"
PR = "r1"
LICENSE = "CLOSED"

inherit obmc-phosphor-systemd

S = "${WORKDIR}"

SRC_URI += "file://bmcready.sh"

RDEPENDS_${PN} = "bash"

BMCREADY_TMPL = "bmc-ready@.service"
BMCREADY_INSTFMT = "bmc-ready@{0}.service"
BMCREADY_FMT = "../${BMCREADY_TMPL}:multi-user.target.wants/${BMCREADY_INSTFMT}"

SYSTEMD_SERVICE_${PN} += "${BMCREADY_TMPL}"

SYSTEMD_LINK_${PN} += "${@compose_list(d, 'BMCREADY_FMT', 'OBMC_CHASSIS_INSTANCES')}"

do_install() {
	install -d ${D}${sbindir}
	install -m 0755 ${WORKDIR}/bmcready.sh ${D}${sbindir}/bmcready.sh
}
