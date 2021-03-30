FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append_olympus-nuvoton  = " file://fw_env.config"

do_install_append_olympus-nuvoton () {
	install -m 644 ${WORKDIR}/fw_env.config ${D}${sysconfdir}/fw_env.config
}
