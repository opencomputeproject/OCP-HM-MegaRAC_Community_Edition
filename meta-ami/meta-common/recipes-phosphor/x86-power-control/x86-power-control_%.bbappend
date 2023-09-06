FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI_EGS:append = "file://0001-Timer-Support-for-Chassis-Systems-Reset.patch"

SRC_URI:append = "${@bb.utils.contains('BBFILE_COLLECTIONS', 'egs', SRC_URI_EGS, '', d)}"
