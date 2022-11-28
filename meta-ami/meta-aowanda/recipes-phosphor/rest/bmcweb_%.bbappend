FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append += "file://0001-rename-SEL-Sensor-objpath.patch"
SRC_URI_append += "file://0002-disabled-sensors-during-powerOff-using-PowerState.patch"
