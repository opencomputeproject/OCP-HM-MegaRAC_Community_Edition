SUMMARY = "CPLD firware update"
DESCRIPTION = "To update CPLD device "
PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "CLOSED"


FILESPATH =. "${TOPDIR}/../openbmc_modules:"
S = "${WORKDIR}/"
SRCPV = "${AUTOREV}"
PACKAGES = "${PN} ${PN}-version"

INSANE_SKIP_${PN} = "dev-so ldflags textrel host-user-contaminated"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT  = "1"


CPLD_MGR_PACKAGES = " \
    ${PN}-version \
"
PACKAGE_BEFORE_PN += "${CPLD_MGR_PACKAGES}"
ALLOW_EMPTY_${PN} = "1"


DBUS_PACKAGES = "${CPLD_MGR_PACKAGES}"

SRC_URI = "file://cpldimage_manager.cpp \
	   file://cpldimage_manager.hpp \
	   file://cpldimage_manager_main.cpp \
	   file://cpldversion.cpp \
	   file://cpldversion.hpp \
	   file://cpldwatch.cpp \
	   file://cpldwatch.hpp \
	   file://elog-errors.hpp \
	   file://meson.build \
	   file://meson_options.txt \
	   file://xyz \
	   file://xyz.openbmc_project.Cpld.Version.conf \
	   file://xyz.openbmc_project.Cpld.Version.service.in \
	   file://cpld.conf \
	  "


SRC_URI += "file://ast-jtag.cpp \
	    file://ast-jtag.hpp \
	    file://lattice.cpp \
	    file://lattice.hpp \
	    file://libpthread.pc \
	   "

# Set SYSTEMD_PACKAGES to empty because we do not want ${PN} and DBUS_PACKAGES
# handles the rest.
SYSTEMD_PACKAGES = ""


inherit meson pkgconfig
inherit obmc-phosphor-dbus-service
inherit python3native

DEPENDS += " \
    openssl \
    phosphor-dbus-interfaces \
    phosphor-logging \
    ${PYTHON_PN}-sdbus++-native \
    sdbusplus \
"
do_configure_prepend (){
	
	cp ${S}/libpthread.pc ${STAGING_LIBDIR}/pkgconfig/
} 

FILES_${PN}-dev += " /usr/bin/.debug/* "
FILES_${PN} += "${bindir}/phosphor-upload-cpldimage ${exec_prefix}/lib/tmpfiles.d/cpld.conf /lib/* /etc/* /usr/include/* "

DBUS_SERVICE_${PN}-version += "xyz.openbmc_project.Cpld.Version.service"


do_populate_sysroot () {
	echo " In sysroot \n  "
	rm -rf ${D}/usr/bin/.debug/ 
}
