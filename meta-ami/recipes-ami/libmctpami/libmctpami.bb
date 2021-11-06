SUMMARY = "AMI mctp api"

SECTION = "lib"

LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = " systemd mctp-wrapper"
RDEPENDS_${PN} = " systemd mctp-wrapper"
ALLOW_EMPTY_${PN} = '1'
INHIBIT_PACKAGE_DEBUG_SPLIT  = "1"
INSANE_SKIP_${PN} = "dev-so dev-deps"
LIB_NAME = "libmctpami"
PACKAGES = "${PN} ${PN}-dev"

SRC_URI = " \
            file://Makefile \
            file://mctp_dbus.c \
            file://mctp_api.h \
        "
S = "${WORKDIR}"


do_install() {
	install -d ${D}${libdir}
	install -d ${D}${includedir}
	oe_soinstall ${LIB_NAME}.so.0.0.0 ${D}${libdir}
    install mctp_api.h ${D}${includedir}
}

FILES_${PN}-dev_append = " ${includedir}/*"
FILES_${PN}_append = " ${libdir}/* "
