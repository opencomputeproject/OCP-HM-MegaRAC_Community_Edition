SUMMARY = "Init BMC Hostname"
DESCRIPTION = "Setup BMC Unique hostname"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${AMIBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit allarch systemd

RDEPENDS_${PN} = "${VIRTUAL-RUNTIME_base-utils}"

SYSTEMD_SERVICE_${PN} = "set-hostname.service"
SYSTEMD_SERVICE_${PN} += "register-dns.service"
SYSTEMD_SERVICE_${PN} += "register-dns.path"

SRC_URI = "file://${PN}.sh file://${PN}.service file://register-dns.service file://register-dns.path"

S = "${WORKDIR}"
do_install() {
    sed "s/{MACHINE}/${MACHINE}/" -i ${PN}.sh
    install -d ${D}${bindir} ${D}${systemd_system_unitdir}
    install ${PN}.sh ${D}${bindir}/
    install -m 644 ${PN}.service ${D}${systemd_system_unitdir}/
    install -m 644 register-dns.service ${D}${systemd_system_unitdir}/
    install -m 644 register-dns.path ${D}${systemd_system_unitdir}/

}

FILES_${PN} = "\
			${systemd_system_unitdir} \
			${bindir} \
			"			
