SUMMARY = "Application to get BIOS POST codes"

SECTION = "Application"

LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
APP_NAME = "bioscode"
localdir = "/usr/local"
bindir = "${localdir}/bin"

SRC_URI = "\
                file://port80.c \
"
DEPENDS = " libsnoop-ex "
RDEPENDS_bioscode-ex = " libsnoop-ex " 
 
S = "${WORKDIR}"
ROOT = "${STAGING_DIR_TARGET}"

do_compile(){
        ${CC} port80.c -o bioscode -I${ROOT}/usr/include/snoop ${LDFLAGS} -lsnoop
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${APP_NAME} ${D}${bindir}

}

FILES_${PN}-dev = ""
FILES_${PN} = "${bindir}/*"
