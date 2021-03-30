FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append += "file://0001-watchdog-timout-events.patch"
SRC_URI_append += "file://0002-watchdog-timer-expiration-actions.patch"
SRC_URI_append += "file://0003-event-generation-fixed.patch"
SRC_URI_append += "file://0004-Added-dont-log-bit.patch"
SRC_URI_append += "file://0005-event-logging-fixed.patch"
SRC_URI_append += "file://0006-Adjust-to-meet-appropriate-transition-state-for-x86-power-control.patch"

# Remove the override to keep service running after DC cycle
SYSTEMD_OVERRIDE_${PN}_remove = "poweron.conf:phosphor-watchdog@poweron.service.d/poweron.conf"
SYSTEMD_SERVICE_${PN} = "phosphor-watchdog.service"
SYSTEMD_LINK_${PN}_remove = "${@compose_list(d, 'WATCHDOG_FMT', 'OBMC_HOST_WATCHDOG_INSTANCES', 'OBMC_HOST_INSTANCES')}"