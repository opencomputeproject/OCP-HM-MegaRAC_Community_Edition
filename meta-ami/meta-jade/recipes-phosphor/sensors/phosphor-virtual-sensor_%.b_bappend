FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI += " \
            file://virtual_sensor_config.json \
            file://0001-Add-catch-exception-when-the-sensor-value.patch \
            file://0002-Support-setting-the-DefaultValue-of-the-absented-sou.patch \
            "

do_install_append_mtjade() {
    install -m 0644 ${WORKDIR}/virtual_sensor_config.json ${D}${datadir}/phosphor-virtual-sensor/
}

