FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
PROJECT_SRC_DIR := "${THISDIR}/${PN}"

SRC_URI += "file://0001-fix-sensornumber-mapping.patch \
            file://0002-Fix-Wrong-SensorName-issue.patch \
    	    file://0003-Add-Severity-Information-For-Discrete-Sensor.patch \
            file://0004-Add-SNMP-Trap-Alert-Support-over-PEF.patch \
        "
DEPENDS += "phosphor-snmp"
