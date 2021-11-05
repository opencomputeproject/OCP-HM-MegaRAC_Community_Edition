POWER_SERVICE_PACKAGES = " \
                         phosphor-power \
                         phosphor-power-psu-monitor \
                         "

#webui-vue
RDEPENDS_${PN}-extras_append_mtjade = " \
                                        ${POWER_SERVICE_PACKAGES} \
                                        phosphor-image-signing \
                                        dbus-sensors \
                                        entity-manager \
                                      "

RDEPENDS_${PN}-inventory_append_mtjade = " \
                                        fault-monitor \
                                        id-button \
                                        tempevent-monitor \
                                        scp-failover \
                                        psu-hotswap-reset \
                                        host-gpio-handling \
                                        "

RDEPENDS_${PN}-extras_remove_mtjade = " phosphor-hwmon"
VIRTUAL-RUNTIME_obmc-sensors-hwmon ?= "dbus-sensors"
#phosphor-virtual-sensor
