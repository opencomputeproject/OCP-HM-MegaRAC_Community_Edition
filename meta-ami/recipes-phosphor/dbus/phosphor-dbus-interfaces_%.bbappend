FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-add-functional-version-property.patch"
SRC_URI += "file://0002-added-properties-to-Watchdog-intf.patch"
SRC_URI += "file://ACBoot.interface.yaml "
SRC_URI += "file://0003-system-boot-options-interfaces.patch"
SRC_URI += "file://0004-added-interface-for-cpld-firmware.patch"
SRC_URI += "file://0005-vlan-priority-property.patch"
SRC_URI += "file://0006-Dhcp-Vendor-Identifier-yaml.patch"
SRC_URI += "file://0028-MCTP-Daemon-D-Bus-interface-definition.patch"
SRC_URI += "file://0007-dbus-passwordPolicy-yaml.patch"
SRC_URI += "file://0008-dbus-passwordPolicy-Update-yaml.patch"
SRC_URI += "file://0009-ARP-Control-property.patch"
SRC_URI += "file://0010-NCSI-Commands-dbus-methods-yaml.patch"
SRC_URI += "file://0011-dbus-IP-active-Property.patch"
SRC_URI += "file://0012-dbus-IP-States-Property.patch"
SRC_URI += "file://0013-dbus-VLAN-Property.patch"

do_configure_prepend(){
    export  SDBUSPP_GEN_MESON=sdbus++-gen-meson
    cp ${WORKDIR}/ACBoot.interface.yaml ${S}/xyz/openbmc_project/Common/
    cd ${S}/gen; ./regenerate-meson
}
