SUMMARY = "AMI NVME Daemon"
LICENSE = "CLOSED" 
PR = "r1"

inherit systemd
inherit autotools pkgconfig cmake

DEPENDS += "autoconf-archive-native \
            systemd phosphor-logging phosphor-dbus-interfaces boost \
            "
DEPENDS +=" libmctp-intel mctpd mctp-wrapper libmctpami"
RDEPENDS_${PN} +=" libmctp-intel mctpd mctp-wrapper libmctpami"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

S = "${WORKDIR}"
SRC_URI += " \ 
	file://CMakeLists.txt \
	file://amiregister.c \
"


#Change Application name, if it is different from recipe name.
APP_NAME = "testmctp"

