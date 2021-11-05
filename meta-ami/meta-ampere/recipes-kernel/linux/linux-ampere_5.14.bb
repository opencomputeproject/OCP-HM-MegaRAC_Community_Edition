KBRANCH ?= "ampere"
LINUX_VERSION ?= "5.14.6"
FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " file://defconfig"

SRCREV="68163de0d4473d62eff58de457c627e37f251c09"

require linux-ampere.inc
