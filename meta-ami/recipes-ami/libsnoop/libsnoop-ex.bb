# Yocto Project Development Manual.
SUMMARY = "snoop library"
SECTION = "Library"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
DEPENDS = " linux-aspeed "
SRC_URI = "file://snoopifc.c \
	   file://snoopifc.h \
	   file://snoop_ioctl.h \
	   file://Makefile \
           "
S = "${WORKDIR}"

CFLAGS_prepend = "-fpic"

EXTRA_OEMAKE = '\
   LIBC="" \
   STAGING_LIBDIR_NATIVE=${STAGING_LIBDIR} \
   STAGING_INCDIR_NATIVE=${STAGING_INCDIR} \
 '

do_install() {
             install -d ${D}${libdir}/
             install -m 0755 libsnoop.so ${D}${libdir}/
   	     install -d ${D}${includedir}/snoop
	     install -m 0644 ${S}/*.h ${D}${includedir}/snoop/
}

FILES_${PN} += "${libdir}/*"
FILES_SOLIBSDEV = ""
INSANE_SKIP_${PN} += "dev-so"
