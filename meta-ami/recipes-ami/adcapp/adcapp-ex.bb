SUMMARY = "ADCAPP"

SECTION = "examples"

LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

APP_NAME = "adcapp"
localdir = "/usr/local"
bindir = "${localdir}/bin"

SRC_URI = "file://adcapp.c"

S = "${WORKDIR}"

do_compile() {
	${CC} adcapp.c ${LDFLAGS} -o adcapp
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 adcapp ${D}${bindir}
	cd ${S}
	install -m 0755 ${APP_NAME} ${D}${bindir}

}

FILES_${PN}-dev = ""
FILES_${PN} = "${bindir}/*"
