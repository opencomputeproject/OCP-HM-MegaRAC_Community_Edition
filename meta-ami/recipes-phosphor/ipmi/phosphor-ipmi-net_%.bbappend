FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
ALT_RMCPP_IFACE  = "eth1"

SRC_URI_append += "file://0001-add-net-service-to-warm-reset-target.patch"
SRC_URI_append += "file://0002-Set-channel-Security.patch"
SRC_URI_append += "file://0003-cipher-suite-privilege-fix.patch"

SYSTEMD_SERVICE_${PN} += " \
    ${PN}@${ALT_RMCPP_IFACE}.service \
    ${PN}@${ALT_RMCPP_IFACE}.socket \
    "

