SUMMARY = "Application to get BIOS POST codes"

SECTION = "Application"

LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

S = "${WORKDIR}"
localdir = "/usr/local"
bindir = "${localdir}/bin"

SRC_URI = "\
                file://port80.c \
"
do_compile(){
 ${CC} port80.c -o bioscode ${LDFLAGS}
}


do_install() {
	install -d ${D}${bindir}
        install ${WORKDIR}/bioscode ${D}${bindir}
}

FILES_${PN}-dev = ""
FILES_${PN} = "${bindir}/*"
