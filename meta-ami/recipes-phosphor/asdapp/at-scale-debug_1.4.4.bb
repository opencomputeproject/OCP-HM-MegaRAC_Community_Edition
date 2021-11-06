inherit obmc-phosphor-systemd

SUMMARY = "At Scale Debug Service"
DESCRIPTION = "At Scale Debug Service exposes remote JTAG target debug capabilities"

LICENSE = "CLOSED"
#LIC_FILES_CHKSUM = "file://LICENSE;md5=0d1c657b2ba1e8877940a8d1614ec560"
INHIBIT_PACKAGE_DEBUG_SPLIT  = "1"
inherit autotools cmake
DEPENDS = "sdbusplus openssl libpam safec libgpiod" 
RDEPENDS_${PN} = "sdbusplus openssl libpam safec libgpiod"

INSANE_SKIP_${PN} += "dev-deps"
#require predefined variables
#require recipes-osp/common/export_build_variables.inc

do_configure[depends] += "virtual/kernel:do_shared_workdir"

SRC_URI = "file://data \
	   file://atScaleDebug.service \
	   file://0001-vc-gpio-configuration.patch \
	  "
PACKAGES = "${PN} ${PN}-dev"
#SRCREV = "d5b67a0e923d12e44158672b8c4cd642f2d27d38"
S = "${WORKDIR}/data"

CFLAGS_append = " -I ${STAGING_KERNEL_DIR}/include/uapi"
CFLAGS_append = " -I ${STAGING_KERNEL_DIR}/include/"
CFLAGS_append = " -I ${STAGING_KERNEL_DIR}/tools/testing/radix-tree/"

SYSTEMD_SERVICE_${PN} += "atScaleDebug.service"
# Specify any options you want to pass to cmake using EXTRA_OECMAKE:
EXTRA_OECMAKE = "-DBUILD_UT=OFF"

