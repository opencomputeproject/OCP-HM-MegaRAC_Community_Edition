FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append += "file://0001-Enable-Discrete-sensor.patch"
SRC_URI_append += "file://0002-Enable-Threshold-sensor.patch"
#SRC_URI_append += "file://0003-Enable-CPU-Sensor.patch"

SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.fansensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.adcsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.exitairsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.ipmbsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.intrusionsensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.psusensor.service"
SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.mcutempsensor.service"
#SYSTEMD_SERVICE_${PN} += " xyz.openbmc_project.cpusensor.service"

SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.gpiosensor.service"
SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.eventsensor.service"
SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.psuevent.service"

