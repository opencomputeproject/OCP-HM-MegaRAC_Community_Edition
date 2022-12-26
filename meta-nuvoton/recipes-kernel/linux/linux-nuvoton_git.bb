#KBRANCH ?= "dev-5.15"
#LINUX_VERSION ?= "5.15.61"

#SRCREV="03008ae8e19dd018de4a3f818de4fa287721de38"

#require linux-nuvoton.inc


KBRANCH ?= "NPCM-5.15-OpenBMC"
LINUX_VERSION ?= "5.15.50"
SRCREV = "683743ebc1db52bd850c3f4dd46770004fd3cbfd"

require linux-nuvoton.inc

SRC_URI:append:nuvoton = " file://0003-i2c-nuvoton-npcm750-runbmc-integrate-the-slave-mqueu.patch"
SRC_URI:append:nuvoton = " file://0004-driver-ncsi-replace-del-timer-sync.patch"
#SRC_URI:append:nuvoton = " file://0015-driver-misc-nuvoton-vdm-support-openbmc-libmctp.patch"
SRC_URI:append:nuvoton = " file://0017-drivers-i2c-workaround-for-i2c-slave-behavior.patch"

# New Arch VDMX/VDMA driver
# SRC_URI:append:nuvoton = " file://2222-driver-misc-add-nuvoton-vdmx-vdma-driver.patch"