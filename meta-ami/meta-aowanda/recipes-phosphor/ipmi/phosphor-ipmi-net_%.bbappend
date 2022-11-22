FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

ALT_RMCPP_IFACE = "eth1"
SYSTEMD_SERVICE_${PN} += " \
    ${PN}@${ALT_RMCPP_IFACE}.service \
    ${PN}@${ALT_RMCPP_IFACE}.socket \
"
