FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append += "file://AOWANDA.json"

do_install_append(){
	install -m 0444 ${S}/configurations/*.json ${D}/usr/share/entity-manager/configurations
}
