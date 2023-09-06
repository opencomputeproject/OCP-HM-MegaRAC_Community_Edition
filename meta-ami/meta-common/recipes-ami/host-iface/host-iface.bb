SUMMARY = "Delete Host Interface User Service"
DESCRIPTION= "This service will monitor the bmcweb.service, if the bmcweb.service reset, then it will delete the host interfaces users on the BMC."
PR = "r1"
PV = "0.1"

# FIXME: when get correct license info
LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

inherit systemd
inherit obmc-phosphor-systemd

SRC_URI = "file://delete-hi-user.service"

DEPENDS = "systemd"
RDEPENDS:${PN} = "bash"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = "delete-hi-user.service"
