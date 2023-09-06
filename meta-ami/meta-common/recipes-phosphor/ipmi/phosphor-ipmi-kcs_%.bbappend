FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "bc7bf463229b69bb2346cc66f1e4b9f65f5374bd"

SRC_URI += " \
	   file://0001-Add-to-warm-reset.patch \
           "
