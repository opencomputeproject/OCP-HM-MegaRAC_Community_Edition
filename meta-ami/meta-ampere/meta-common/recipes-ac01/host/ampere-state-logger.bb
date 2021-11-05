LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=5f4ed2144f2ed8db87f4f530d9f68710"
inherit systemd meson pkgconfig

DEPENDS = "boost sdbusplus libgpiod systemd phosphor-dbus-interfaces phosphor-logging"
RDEPENDS_${PN} += "libsystemd bash"

S = "${WORKDIR}/git/state-logger"

SRC_URI = "git://github.com/ampere-openbmc/ampere-misc.git;protocol=https;branch=ampere"
SRCREV = "d2d6d00bfa4a252a86e7170b15359b7e8670bd8f"

SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.state_logger.service"
