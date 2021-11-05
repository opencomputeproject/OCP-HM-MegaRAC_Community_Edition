SUMMARY = "Phosphor OpenBMC Delete Bootstrap Account Handling Service"
DESCRIPTION = "Phosphor OpenBMC Altra Delete Bootstrap Account Handling Daemon"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit systemd
inherit obmc-phosphor-systemd

DEPENDS += "systemd"
RDEPENDS_${PN} += "libsystemd"
RDEPENDS_${PN} += "bash"

SRC_URI = "file://ampere_delete_bootstrap_account.sh"

SYSTEMD_PACKAGES = "${PN}"

HOST_ON_RESET_HOSTTMPL = "xyz.openbmc_project.deletebootstrapaccount.service"
HOST_ON_RESET_HOSTINSTMPL = "xyz.openbmc_project.deletebootstrapaccount.service"
HOST_ON_RESET_HOSTTGTFMT = "obmc-host-startmin@{0}.target"
HOST_ON_RESET_HOSTFMT = "../${HOST_ON_RESET_HOSTTMPL}:${HOST_ON_RESET_HOSTTGTFMT}.requires/${HOST_ON_RESET_HOSTINSTMPL}"
SYSTEMD_LINK_${PN} += "${@compose_list_zip(d, 'HOST_ON_RESET_HOSTFMT', 'OBMC_HOST_INSTANCES')}"

SYSTEMD_SERVICE_${PN} += "${HOST_ON_RESET_HOSTTMPL}"

do_install () {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/ampere_delete_bootstrap_account.sh ${D}${sbindir}/
}

