SUMMARY = "PECI application"
SECTION = "apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

APP_NAME = "peciapp"
localdir = "/usr/local"
bindir = "${localdir}/bin"

SRC_URI = "file://peciapp.c \
	   file://peciifc.c \
	   file://crc8.c \
	   file://peciifc.h \
	   file://checksum.h \
	   file://crc8.h "

S = "${WORKDIR}"

do_compile() {
        ${CC} peciifc.c crc8.c peciapp.c ${LDFLAGS} -o peciapp
        
}

do_install () {
    install -m 0755 -d ${D}${localdir}
    install -m 0755 -d ${D}${bindir}
    cd ${S}
    install -m 0755 ${APP_NAME} ${D}${bindir}
}

FILES_${PN}-dev = ""
FILES_${PN} = "${bindir}/*"
