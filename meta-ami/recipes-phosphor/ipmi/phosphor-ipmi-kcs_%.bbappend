FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-add-kcs-service-to-warm-reset-target.patch"
SRC_URI += "file://0002-kcsbridge-servicename-change.patch"
