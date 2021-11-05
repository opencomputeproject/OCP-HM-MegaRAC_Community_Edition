FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI += " \
            file://Mt_Jade.json \
            file://Mt_Jade-Zone.json \
            file://blacklist.json \
            file://xyz.openbmc_project.FruDevice.service \
            file://0002-FruDevice-Support-parsing-Mt.Jade-motherboard-EEPROM.patch \
           "

do_install_append_mtjade() {
     install -d ${D}${datadir}/${PN}
     install -m 0444 ${WORKDIR}/blacklist.json ${D}${datadir}/${PN}
     install -d ${D}${datadir}/${PN}/configurations
     install -m 0444 ${WORKDIR}/Mt_Jade.json ${D}${datadir}/${PN}/configurations
     install -m 0444 ${WORKDIR}/Mt_Jade-Zone.json ${D}${datadir}/${PN}/configurations

     install -d ${D}${systemd_system_unitdir}
     install -m 0644 ${WORKDIR}/xyz.openbmc_project.FruDevice.service ${D}${systemd_system_unitdir}
}
