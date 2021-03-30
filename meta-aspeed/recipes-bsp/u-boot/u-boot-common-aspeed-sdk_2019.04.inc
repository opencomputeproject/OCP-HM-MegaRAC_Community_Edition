HOMEPAGE = "https://github.com/AspeedTech-BMC/u-boot"
SECTION = "bootloaders"
DEPENDS += "flex-native bison-native"

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=30503fd321432fc713238f582193b78e"
PE = "1"

# We use the revision in order to avoid having to fetch it from the
# repo during parse

#PV = "v2019.04"
#S = "${WORKDIR}/u-boot-2019.04"
#PACKAGE_SRC_NAME = "Bootloader_Pristine_ex-src"
#FILESPATH =. "${TOPDIR}/../bootloader/packages/Bootloader_20XX_Pristine-src/data:"
#SRC_URI = "file://u-boot-2019.04.tar.bz2"

PV = "v2019.04"
S = "${WORKDIR}/u-boot-sdk600"
FILESPATH =. "${TOPDIR}/../openbmc_modules:"
SRC_URI = "file://u-boot-sdk600.tar.bz2"

