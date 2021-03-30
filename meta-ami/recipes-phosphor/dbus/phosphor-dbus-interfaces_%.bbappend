FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-add-property.patch"
SRC_URI += "file://0002-Add-chassis-reset-to-Chassis-State.patch \
	    file://0003-added-interface-for-cpld-firmware.patch \
	    file://0004-initial-functionality-for-get-set-system-boot-option.patch \
	    file://0005-added-properties-to-Watchdog-intf.patch \
            file://0006-Dhcp-Vendor-Identifier-yaml.patch \
	    file://0007-boot-parameter-validity-status.patch"

SRC_URI += "file://ACBoot.interface.yaml "

do_configure_prepend(){
    cp ${WORKDIR}/ACBoot.interface.yaml ${S}/xyz/openbmc_project/Common/
}
