FILESEXTRAPATHS_append_mtjade := "${THISDIR}/${PN}:"

SRC_URI += " \
             file://0001-Correct-the-power-button-press-time.patch \
             file://0002-button-handler-Add-button-event-log.patch \
           "
