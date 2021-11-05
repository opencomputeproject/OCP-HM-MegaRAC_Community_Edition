KBRANCH ?= "ampere"
LINUX_VERSION ?= "5.10.23"
FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRCREV="a76d8033f6fed6357a72ce14aa59ab74bce0658a"

require linux-ampere.inc

SRC_URI += " file://defconfig"

# 5.14 flash layout
# SRC_URI += " file://0001-ARM-dts-aspeed-Grow-u-boot-partition-64MiB-OpenBMC-f.patch"
