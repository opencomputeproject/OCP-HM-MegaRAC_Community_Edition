FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"
SRC_URI_append += "file://0001-Power-config.patch"
SRC_URI_append += "file://0002-powercyclems-modified.patch"
