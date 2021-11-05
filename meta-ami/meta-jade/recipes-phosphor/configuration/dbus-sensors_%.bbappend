FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
#file://0001-ADCSensor-Add-PollRate-and-fix-PowerState-Always.patch
#file://0004-Dbus-sensors-restructure-the-code-handle-PowerState-.patch
#file://0005-ADCSensor-FanSensor-Support-ChassisState-attribute.patch
#file://0006-FanSensor-Add-xyz.openbmc_project.Control.FanPwm-to-.patch

#SRC_URI += " \
#            file://0002-ADCSensor-Use-device-name-to-match-the-ADC-sensors.patch \
#            file://0003-ADCSensor-Add-support-DevName-option.patch \
#            file://0007-AmpereSoc-Add-AmpereSoC-daemon-to-public-AmpereSoC-d.patch \
#            file://0008-AmpereSoc-Rescan-SoC-sensors-in-the-chassis-host-pow.patch \
#            file://0009-AmpereSoc-Handle-PowerState-option.patch \
#            file://0010-AmpereSoc-Support-CPU-Present-properties-in-socsenso.patch \
#            file://0011-AmpereSoC-Remove-propertyChanged-signal-handling-for.patch \
#            file://0012-Change-the-matched-string-of-the-host-running-state-.patch \
#            file://0013-amperesoc-Fix-the-host-sensors-are-nan-after-the-hos.patch \
#           "

SRC_URI += " \
            file://0001-ADCSensors-devicename-match-DevName.patch \
           "

PACKAGECONFIG_mtjade = " \
                        adcsensor \
                        fansensor \
                        hwmontempsensor \
                        psusensor \
                        socsensor \
                        "
PACKAGECONFIG[socsensor] = "-Dsoc=enabled, -Dsoc=disabled"
SYSTEMD_SERVICE_${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'socsensor', \
                                               'xyz.openbmc_project.socsensor.service', \
                                               '', d)}"
