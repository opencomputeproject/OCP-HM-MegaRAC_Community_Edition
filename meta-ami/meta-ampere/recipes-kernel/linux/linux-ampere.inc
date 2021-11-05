DESCRIPTION = "Linux kernel for Ampere"
SECTION = "kernel"
LICENSE = "GPLv2"

PROVIDES += "virtual/kernel"

KCONFIG_MODE="--alldefconfig"

KSRC ?= "git://github.com/ampere-openbmc/linux;protocol=git;branch=${KBRANCH}"
SRC_URI = "${KSRC}"

LINUX_VERSION_EXTENSION ?= "-${SRCREV}"

PV = "${LINUX_VERSION}+git${SRCPV}"

KERNEL_FEATURES_remove = "phosphor-gpio-keys"

inherit kernel
require recipes-kernel/linux/linux-yocto.inc

# From 5.6+ the COPYING file changed
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"
