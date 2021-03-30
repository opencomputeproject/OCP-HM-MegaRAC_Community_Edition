FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append += "file://checkFru.sh"
SRC_URI_append += "file://Tiogapass.fru.bin"

do_install_append () {
 cp ${S}/Tiogapass.fru.bin ${D}/${sysconfdir}/fru
}
