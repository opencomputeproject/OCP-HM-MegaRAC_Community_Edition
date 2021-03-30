HOMEPAGE = "https://github.com/openbmc/ipmi-fru-parser"
LICENSE = "GPL-3.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7702f203b58979ebbc31bfaeb44f219c"
//SRC_URI += "git://github.com/openbmc/ipmi-fru-parser"
//SRCREV = "0968237b479d649ecaac7561cf07fbacf241d98c"

FILESPATH =. "${TOPDIR}/../openbmc_modules:"
S = "${WORKDIR}/ipmi-fru-parser"
SRC_URI = "file://ipmi-fru-parser"
SRCPV = "${AUTOREV}"

