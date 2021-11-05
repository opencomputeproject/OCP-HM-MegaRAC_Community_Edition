FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

EXTRA_OEMESON_append = " \
			-Dsupply-monitor=false \
		       "

SYSTEMD_SERVICE_${PN}-monitor_remove = "power-supply-monitor@.service"

SRC_URI += "file://psu_config.json"
SRC_URI += "file://0001-psu-ng-Remove-the-define-IBM_VPD.patch"

do_install_append() {
    install -D ${WORKDIR}/psu_config.json ${D}${datadir}/phosphor-psu-monitor/psu_config.json
}

FILES_${PN} += "${datadir}/phosphor-power/psu_config.json"
