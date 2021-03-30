SUMMARY = "Tiogaspass Power On"
PR = "r1"
LICENSE = "CLOSED"

inherit obmc-phosphor-systemd

S = "${WORKDIR}"

SRC_URI += "file://poweron.sh"

RDEPENDS_${PN} = "bash"

POWERON_TMPL = "power-on@.service"
POWERON_INSTFMT = "power-on@{0}.service"
POWERON_FMT = "../${POWERON_TMPL}:multi-user.target.wants/${POWERON_INSTFMT}"

SYSTEMD_SERVICE_${PN} += "${POWERON_TMPL}"

SYSTEMD_LINK_${PN} += "${@compose_list(d, 'POWERON_FMT', 'OBMC_CHASSIS_INSTANCES')}"

do_install() {
	install -d ${D}${sbindir}
	install -m 0755 ${WORKDIR}/poweron.sh ${D}${sbindir}/poweron.sh
}
