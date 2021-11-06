FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append += "file://0001-scale-prop.patch"
SRC_URI_append += "file://0002-nm-sensor.patch"
SRC_URI_append += "file://0003-disable-rtti.patch"
