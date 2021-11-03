SUMMARY = "Host interface application"
SECTION = "examples"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI_append += "file://CMakeLists.txt"
SRC_URI_append += "file://src/"
SRC_URI_append += "file://include/"
SRC_URI_append += "file://service_files/"

DEPENDS = "boost nlohmann-json sdbusplus"

S = "${WORKDIR}"

inherit cmake systemd

SYSTEMD_SERVICE_${PN} = "Hostinventory.service"

#FILES_${PN} += "/usr/bin/"
#FILES_${PN} += "/lib/systemd/system/"

do_install() {
         install -d ${D}${bindir}
         install -m 0755 hostinv ${D}${bindir}
	 cp ${WORKDIR}/src/configure_usb_gadget.sh  ${D}${bindir}

         install -d ${D}${base_libdir}/systemd/system
         install -m 0644 ${S}/service_files/Hostinventory.service ${D}${base_libdir}/systemd/system

         #install -d ${D}{systemd_unitdir}/system
         #install -m 0644 ${WORKDIR}/xyz.openbmc_project.hostinventory.service ${D}{systemd_unitdir}/system/
}
