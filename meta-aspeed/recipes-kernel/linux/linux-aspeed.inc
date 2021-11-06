DESCRIPTION = "Linux kernel for Aspeed"
SECTION = "kernel"
LICENSE = "GPLv2"

PROVIDES += "virtual/kernel"

KCONFIG_MODE="--alldefconfig"

#KSRC ?= "git://github.com/openbmc/linux;protocol=git;branch=${KBRANCH}"
#SRC_URI = "${KSRC}"
#SRC_URI += " file://defconfig"

#LINUX_VERSION_EXTENSION ?= "-${SRCREV}"

#PV = "${LINUX_VERSION}+git${SRCPV}"

PV = "${LINUX_VERSION}"
S = "${WORKDIR}/linux-5.4.85"
FILESPATH =. "${TOPDIR}/../kernel/packages/Kernel_5_Pristine-src/data:"
SRC_URI = "file://linux-5.4.85.tar.bz2"
SRC_URI += " file://defconfig"

inherit kernel
require recipes-kernel/linux/linux-yocto.inc

# From 5.6+ the COPYING file changed
LIC_FILES_CHKSUM = "file://COPYING;md5=bbea815ee2795b2f4230826c0c6b8814"
