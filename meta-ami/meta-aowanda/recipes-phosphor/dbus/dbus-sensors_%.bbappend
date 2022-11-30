FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"


PACKAGECONFIG ??= " \
    adcsensor \
    cpusensor \
    exitairtempsensor \
    fansensor \
    hwmontempsensor \
    intrusionsensor \
    ipmbsensor \
    mcutempsensor \
    psusensor \
    "

PACKAGECONFIG[adcsensor] = "-DDISABLE_ADC=OFF, -DDISABLE_ADC=ON"
PACKAGECONFIG[cpusensor] = "-DDISABLE_CPU=OFF, -DDISABLE_CPU=ON"
PACKAGECONFIG[exitairtempsensor] = "-DDISABLE_EXIT_AIR=OFF, -DDISABLE_EXIT_AIR=ON"
PACKAGECONFIG[fansensor] = "-DDISABLE_FAN=OFF, -DDISABLE_FAN=ON"
PACKAGECONFIG[hwmontempsensor] = "-DDISABLE_HWMON_TEMP=OFF, -DDISABLE_HWMON_TEMP=ON"
PACKAGECONFIG[intrusionsensor] = "-DDISABLE_INTRUSION=OFF, -DDISABLE_INTRUSION=ON"
PACKAGECONFIG[ipmbsensor] = "-DDISABLE_IPMB=OFF, -DDISABLE_IPMB=ON"
PACKAGECONFIG[mcutempsensor] = "-DDISABLE_MCUTEMP=OFF, -DDISABLE_MCUTEMP=ON"
PACKAGECONFIG[psusensor] = "-DDISABLE_PSU=OFF, -DDISABLE_PSU=ON"

SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'adcsensor', \
                                               'xyz.openbmc_project.adcsensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'cpusensor', \
                                               'xyz.openbmc_project.cpusensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'exitairtempsensor', \
                                               'xyz.openbmc_project.exitairsensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'fansensor', \
                                               'xyz.openbmc_project.fansensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'hwmontempsensor', \
                                               'xyz.openbmc_project.hwmontempsensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'intrusionsensor', \
                                               'xyz.openbmc_project.intrusionsensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'ipmbsensor', \
                                               'xyz.openbmc_project.ipmbsensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'mcutempsensor', \
                                               'xyz.openbmc_project.mcutempsensor.service', \
                                               '', d)}"
SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'psusensor', \
                                               'xyz.openbmc_project.psusensor.service', \
                                               '', d)}"
