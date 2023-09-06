FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "git://git.ami.com/core/ami-bmc/one-tree/core/phosphor-dbus-interfaces.git;branch=main;protocol=https;name=override;"
SRCREV_FORMAT = "override"
SRCREV_override = "84b66bfacbdbf3ad5946c9e1b6024306554dce04"

SRC_URI += "file://0001-ARP-Control-property.patch\
	    file://0003-ARP-VLAN-YAML.patch \
            file://0036-EnhancedPasswordPolicy.patch \
	    file://0005-Add-Bootstrap-credential-support.patch \
            file://0006-Add-Diag-Arugment-in-Boot-Mode-Interface.patch \
            file://0008-Add-Prefix-Length-at-Neighbor.patch \
            file://0009-Implement-EIP-741000.-Implement-DDNS-Nsupdate-Featur.patch \
            file://0010-Add-DBus-Property-IPv4-IPv6-Enabled-Disabled-And-Error-Handling.patch \
            file://0010-Added-TimeOut-for-managers.patch \
	    "
EXTRA_OEMESON += "-Ddata_com_intel=true"
