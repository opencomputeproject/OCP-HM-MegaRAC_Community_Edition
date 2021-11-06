FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://rsyslog-conf \
"

FILES_${PN} += "/var/sellog"

DEPENDS+ = "rsyslog-rotate"

EXTRA_OECONF += "--enable-omprog"

PACKAGECONFIG_append = " imjournal"
PACKAGECONFIG_append = " omprog"

do_install_append() {
        install -m 0644 ${WORKDIR}/rsyslog-conf ${D}/etc/rsyslog.conf
        install -d ${D}/var/sellog
	rm ${D}${sysconfdir}/rsyslog.d/imjournal.conf
}

