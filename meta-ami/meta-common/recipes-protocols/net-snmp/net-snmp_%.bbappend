FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://0001-Add-noreturn-attribute-to-netsnmp_pci_error.patch \
    file://CVE-2022-44792-CVE-2022-44793.patch \
    "
