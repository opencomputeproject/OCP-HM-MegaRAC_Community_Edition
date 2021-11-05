FILESEXTRAPATHS_prepend_mtjade := "${THISDIR}/${PN}:"

SRC_URI += " file://nvme_config.json"

#SRC_URI += " 
#file://0003-sdbusplus-Remove-the-Error-log-in-SMBus-command-send.patch
#"

#file://0001-Ampere-phosphor-nvme-Remove-reading-VPD-device.patch
#file://0002-nvme_manager-Add-checking-the-changing-before-set-in.patch
do_install_append() {
    install -m 0644 -D ${WORKDIR}/nvme_config.json \
                   ${D}/etc/nvme
}
