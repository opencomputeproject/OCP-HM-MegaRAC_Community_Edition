FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://tiogapass.cfg"
SRC_URI += "file://001-tiogapass.dts.patch"
SRC_URI += "file://002-Updated-tiogapass-dts.patch"
SRC_URI += "file://003-Enabled-PECI.patch"
SRC_URI += "file://004-image-bmc-lable-added.patch"
SRC_URI += "file://005-removed-pwroklines.patch"
