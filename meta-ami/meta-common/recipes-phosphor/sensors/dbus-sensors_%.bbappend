FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "1e3842541db673c850625bfae4227fb88767a2a9"

SRC_URI:append = " \
            file://0001-converted-index-to-0-based-and-made-pwm-starts-from-.patch \
	    file://0002-Add-Processor-Type-Sensor-Support.patch \
            file://0003-ProcessorSensor-Replace-iterator-pairs-with-structur.patch \
	    file://0004-Add-Watchdog2-Discrete-Sensor.patch \
	    file://0005-Add-Severity-Information-For-Discrete-Sensor.patch \
	    file://0006-disable-unsupported-sensors.patch\
	    file://0007-Update-ObjectManager-for-sensors-in-Right-Path.patch \
        file://0009-Fix-for-Fan-Redundancy.patch \
            "

PACKAGECONFIG[processorstatus] = "-Dprocstatus=enabled, -Dprocstatus=disabled"
PACKAGECONFIG[systemsensor] = "-Dsystem=enabled, -Dsystem=enabled"

PACKAGECONFIG:append += "processorstatus \
			 systemsensor \
"
SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'processorstatus', \
                                               'xyz.openbmc_project.processorstatus.service', \
                                               '', d)}"

SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'systemsensor', \
                                               'xyz.openbmc_project.systemsensor.service', \
                                               '', d)}"

