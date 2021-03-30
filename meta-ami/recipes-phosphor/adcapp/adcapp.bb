SUMMARY = "ADCAPP"
SECTION = "examples"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

APP_NAME = "adcapp"
bindir = "/usr/bin"

SRC_URI = "git://github.com/openbmc/openbmc-tools"
SRC_URI += "file://01-adcapp.patch"

SRCREV = "cb66a9e779982aba96373475e99a6d8f296362d5"

S = "${WORKDIR}/git/hongweiz/adcapp/src"
PV = "0.1+git${SRCPV}"

do_compile() {
	${CC} adcapp.c adcifc.c EINTR_wrappers.c ${LDFLAGS} -o adcapp
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 adcapp ${D}${bindir}
	cd ${S}
	install -m 0755 ${APP_NAME} ${D}${bindir}

}

FILES_${PN}-dev = ""
FILES_${PN} = "${bindir}/*"
