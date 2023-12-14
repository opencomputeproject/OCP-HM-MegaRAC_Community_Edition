FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "b9f7c1de73ad44c4c6ced7dab8b2a402468cec5d"

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
