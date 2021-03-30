SUMMARY = "ipmbd tx/rx daemon"
DESCRIPTION = "The ipmb daemon to receive/transmit messages"
SECTION = "base"
PR = "r2"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmbd.c;beginline=4;endline=16;md5=aff63587edbd0791be7fba837223394c"

inherit obmc-phosphor-systemd

SRC_URI = "file://Makefile \
           file://ipmbd.c \
           file://ipmbd.h \
           file://i2c.h \
           file://obmc-ipmbd.service \
          "

S = "${WORKDIR}"
DEPENDS += "update-rc.d-native"

SYSTEMD_SERVICE_${PN} = " obmc-ipmbd.service"

binfiles = "ipmbd"

pkgdir = "ipmbd"

do_install() {
	install -d ${D}/usr/bin/
	install -d ${D}${sysconfdir}/systemd/system
	install -m 0755 ${S}/ipmbd ${D}/usr/bin/
	install -m 0644 ${WORKDIR}/obmc-ipmbd.service ${D}${sysconfdir}/systemd/system	

}
FILES_${PN}-dev = ""
FILES_${PN} = "/usr/bin/* ${sysconfdir}/*"
