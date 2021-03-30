SUMMARY = "This package includes the binary for PWMTACH test application"
SECTION = "apps"
LICENSE = "CLOSED"

APP_NAME = "pwmtachtool"
localdir = "/usr/local"
bindir = "${localdir}/bin"

SRC_URI = "file://pwmtachtool.c \
           file://pwmtach.c \
           file://libpwmtach.h \
           file://Makefile \
           file://pwmtach_ioctl.h \
           file://EINTR_wrappers.h \
           file://EINTR_wrappers.c \
	   "

S = "${WORKDIR}"

do_compile() {
    make -f Makefile
        
}

do_install () {
    install -m 0755 -d ${D}${localdir}
    install -m 0755 -d ${D}${bindir}
    cd ${S}
    install -m 0755 ${APP_NAME} ${D}${bindir}
}

FILES_${PN}-dev = ""
FILES_${PN} = "${bindir}/*"
