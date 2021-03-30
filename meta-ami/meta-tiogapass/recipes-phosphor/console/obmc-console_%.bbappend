FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}/${MACHINE}:"
OBMC_CONSOLE_HOST_TTY = "ttyS1"

SRC_URI_append += "file://server.ttyS1.conf"
SRC_URI_remove = "file://${BPN}.conf"

do_install_append() {
        install -m 0755 -d ${D}${sysconfdir}/${BPN}
        rm -f ${D}${sysconfdir}/${BPN}/server.ttyVUART0.conf
        install -m 0644 ${WORKDIR}/server.ttyS1.conf ${D}${sysconfdir}/${BPN}
}
