SUMMARY = "ADCAPP"
SECTION = "examples"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

APP_NAME = "adcapp"
bindir = "/usr/bin"

SRC_URI = "git://github.com/openbmc/openbmc-tools;protocol=https;branch=master"


SRCREV = "12c5f115dcf5bcb3983e53f3e1e8bb39ab0f1d98"

S = "${WORKDIR}/git/adcapp/src"
PV = "0.1+git${SRCPV}"
CFLAGS:append =" -DAST2600_ADCAPP"

do_compile() {
        ${CC} adcapp.c adcifc.c EINTR_wrappers.c ${LDFLAGS} ${CFLAGS} -o adcapp
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 adcapp ${D}${bindir}
	cd ${S}
	install -m 0755 ${APP_NAME} ${D}${bindir}

}

FILES_${PN}-dev = ""
FILES_${PN} = "${bindir}/*"
