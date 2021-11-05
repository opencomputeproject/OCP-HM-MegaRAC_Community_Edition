FILESEXTRAPATHS_append := "${THISDIR}/${PN}:"

#file://0001-mtjade-phosphor-fan-presence-Fix-condition-to-check-.patch

SRC_URI += " \
            file://0002-mtjade-support-action-to-set-speed-from-max-sensor-r.patch \
            file://0003-mtjade-expand-the-support-themal-group.patch \
            file://phosphor-fan-control-init@.service \
            file://phosphor-fan-monitor-init@.service \
           "

do_install_append() {
  install -d ${D}${systemd_system_unitdir}
  install -m 0644 ${WORKDIR}/phosphor-fan-monitor-init@.service ${D}${systemd_system_unitdir}
  install -m 0644 ${WORKDIR}/phosphor-fan-control-init@.service ${D}${systemd_system_unitdir}
}
