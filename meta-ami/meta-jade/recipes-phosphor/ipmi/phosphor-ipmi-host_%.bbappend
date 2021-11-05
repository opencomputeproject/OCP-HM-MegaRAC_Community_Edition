FILESEXTRAPATHS_append_mtjade := "${THISDIR}/${PN}:"

DEPENDS_append_mtjade = " mtjade-yaml-config"

RRECOMMENDS_${PN} += "ipmitool"
RDEPENDS_${PN} += "bash"

EXTRA_OECONF_mtjade = " \
    SENSOR_YAML_GEN=${STAGING_DIR_HOST}${datadir}/mtjade-yaml-config/ipmi-sensors-${MACHINE}.yaml \
    FRU_YAML_GEN=${STAGING_DIR_HOST}${datadir}/mtjade-yaml-config/ipmi-fru-read.yaml \
    "

#            file://0002-Implement-the-set-get-system-boot-option-parameters.patch
SRC_URI += "file://0001-mtjade-FRU-Updated-the-phosphor-host-ipmid-to-handle.patch \
            file://0003-Correct-ipmitool-get-system-guid.patch \
            file://0004-Add-the-user_mgmt.hpp-to-Makefile.am-file-for-access.patch \
            file://0005-correct-the-hard-reset-command.patch \
            file://0006-Response-the-thresholds-of-the-sensors-with-some-un-.patch \
            file://ampere-phosphor-softpoweroff \
            file://ampere.xyz.openbmc_project.Ipmi.Internal.SoftPowerOff.service \
            file://0007-sensor-Response-thresholds-for-Get-SDR-command.patch \
            file://0008-Change-revision-to-decimal-number.patch \
            file://0009-jade-update-restartcause-dbus-info.patch \
            "

AMPERE_SOFTPOWEROFF_TMPL = "ampere.xyz.openbmc_project.Ipmi.Internal.SoftPowerOff.service"

do_install_append_mtjade(){
    install -d ${D}${includedir}/phosphor-ipmi-host
    install -m 0644 -D ${S}/sensorhandler.hpp ${D}${includedir}/phosphor-ipmi-host
    install -m 0644 -D ${S}/selutility.hpp ${D}${includedir}/phosphor-ipmi-host
    install -m 0755 ${WORKDIR}/ampere-phosphor-softpoweroff ${D}/${bindir}/phosphor-softpoweroff
    install -m 0644 ${WORKDIR}/${AMPERE_SOFTPOWEROFF_TMPL} ${D}${systemd_unitdir}/system/xyz.openbmc_project.Ipmi.Internal.SoftPowerOff.service
}
