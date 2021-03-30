FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-add-ipmb-service-to-warm-reset-target.patch"
SRC_URI += "file://0002-Add-dbus-method-SlotIpmbRequest.patch"
SRC_URI += "file://0003-configure-me-device.patch"

