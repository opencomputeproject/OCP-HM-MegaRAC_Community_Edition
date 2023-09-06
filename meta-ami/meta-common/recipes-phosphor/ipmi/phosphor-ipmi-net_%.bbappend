FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "67aaec2e06ff9c2bfd5900b9f07834fca9d009f2"

SRC_URI += " \
	   file://0013-Add-to-warm-reset.patch \
           file://0014-Postpone-To-Wait-Network-Service.patch \
           "
