FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append += "file://0001-threshold-event-for-sensor.patch"
SRC_URI_append += "file://0002-discrete-event-log.patch"
SRC_URI_append += "file://0003-added-ipmi-threshold-event-type.patch"
SRC_URI_append += "file://0004-multiple-state-discrete-sensor.patch"
SRC_URI_append += "file://0005-avoid-duplicate-threshold-event-log.patch"

do_install_append(){
   install -d ${D}/var
   install -d ${D}/var/event
}

