FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append += "file://checkFru.sh"
SRC_URI_append += "file://aowanda.fru.bin"

