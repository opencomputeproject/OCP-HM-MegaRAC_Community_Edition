SUMMARY = "Ampere Computing LLC SCP failover"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit obmc-phosphor-systemd
inherit allarch

RDEPENDS_${PN} += "bash"

DEPENDS += "virtual/obmc-gpio-monitor"
RDEPENDS_${PN} += "virtual/obmc-gpio-monitor"

OBMC_HOST_MONITOR_INSTANCES = "s0_scp_auth_failure"

# Copies config file having arguments
# via GPIO assertion
SYSTEMD_ENVIRONMENT_FILE_${PN} +="obmc/gpio/s0_scp_auth_failure \
                                 "

SRC_URI += " file://scp_failover.sh"

do_install() {
        install -d ${D}${bindir}
        install -m 0755 ${WORKDIR}/scp_failover.sh \
        ${D}${bindir}/scp_failover.sh
}

SYSTEMD_SERVICE_${PN} ?= "scp_failover.service"

GPIO_MONITOR_TMPL = "phosphor-gpio-monitor@.service"
GPIO_MONITOR_TGTFMT = "phosphor-gpio-monitor@{0}.service"
TGT = "multi-user.target"
MONITOR_FMT = "../${GPIO_MONITOR_TMPL}:${TGT}.requires/${GPIO_MONITOR_TGTFMT}"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'MONITOR_FMT', 'OBMC_HOST_MONITOR_INSTANCES')}"
