FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://rsyslog-ampere.conf \
           "

PACKAGECONFIG_append = " imjournal"

do_install_append() {
        install -d ${D}${bindir}
	install -m 0644 ${WORKDIR}/rsyslog-ampere.conf ${D}/etc/rsyslog.conf
}
