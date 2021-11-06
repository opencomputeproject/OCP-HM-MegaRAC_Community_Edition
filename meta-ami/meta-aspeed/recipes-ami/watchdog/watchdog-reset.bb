FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SUMMARY = "Stop Watchdog"
DESCRIPTION = "Stop watchdog after system is up"

LICENSE = "CLOSED"

inherit cmake systemd

SRC_URI = "file://CMakeLists.txt \
	file://src/watchdog-reset.c \
	file://watchdog-reset.service \
	"

DEPENDS = ""

SYSTEMD_SERVICE_${PN} = "watchdog-reset.service"

FILES_${PN}_append = "${bindir}/watchdog-reset"
FILES_${PN}_append += "${systemd_system_unitdir}/watchdog-reset.service"

S = "${WORKDIR}"

do_install_append() {
	install -d "${D}/${systemd_system_unitdir}"
	install -m 0644 "${S}/watchdog-reset.service" "${D}/${systemd_system_unitdir}"
}
