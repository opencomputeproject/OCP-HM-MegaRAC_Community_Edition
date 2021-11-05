LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit systemd meson pkgconfig

DEPENDS = "boost sdbusplus systemd phosphor-dbus-interfaces phosphor-logging nlohmann-json gpioplus"

RDEPENDS_${PN} += "libsystemd bash"

EXTRA_OEMESON_append = " \
    -Derror-monitor=enabled \
    -Dpower-limit=enabled \
    "

S = "${WORKDIR}/git"

SRC_URI = "git://github.com/ampere-openbmc/ampere-platform-mgmt.git;protocol=https;branch=ampere"
SRCREV = "d016a159cd5649a68fdd982264d2fe80895d18e4"
SRC_URI += " file://platform-config.json"


SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.ampere_host_error_monitor.service"
SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.AmpSocPower.service"
SYSTEMD_LINK_${PN} += "${@compose_list(d, 'FMT_MNT', 'OBMC_CHASSIS_INSTANCES')}"

do_install_append_mtjade() {
    install -d ${D}${datadir}/${PN}
    install -m 0644 -D ${WORKDIR}/platform-config.json \
        ${D}${datadir}/${PN}/config.json
}
