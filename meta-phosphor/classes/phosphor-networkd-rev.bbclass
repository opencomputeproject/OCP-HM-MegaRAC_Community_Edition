#SRC_URI += "git://github.com/openbmc/phosphor-networkd"
#SRCREV = "7f9c6680ab9cbc215458dfacc89899f20a8ba50d"

FILESPATH =. "${TOPDIR}/../openbmc_modules:"
S = "${WORKDIR}/phosphor-networkd"
SRC_URI = "file://phosphor-networkd"
SRCPV = "${AUTOREV}"

