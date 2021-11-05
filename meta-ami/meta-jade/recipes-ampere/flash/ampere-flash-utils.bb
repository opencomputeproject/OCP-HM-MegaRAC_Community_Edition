
SUMMARY = "Ampere Computing LLC Flashing Utilities"
DESCRIPTION = "Application to support flashing utilities on Ampere platforms"
PR = "r0"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

RDEPENDS_${PN} = "bash"
DEPENDS = "zlib"

SRC_URI = "git://github.com/ampere-openbmc/ampere-platform-mgmt.git;protocol=https;branch=ampere"
SRCREV = "6a4fb5236d4f413c1c5828c5f7511303c335de26"

SRC_URI += "\
            file://ampere_firmware_upgrade.sh \
            file://ampere_flash_bios.sh \
            "

S = "${WORKDIR}/git/utilities/flash"
ROOT = "${STAGING_DIR_TARGET}"

LDFLAGS += "-L ${ROOT}/usr/lib/ -lz "

do_compile_append() {
    ${CC} ${S}/ampere_eeprom_prog.c -o ${S}/ampere_eeprom_prog -I${ROOT}/usr/include/ ${LDFLAGS}
    ${CC} ${S}/nvparm.c -o ${S}/nvparm -I${ROOT}/usr/include/ ${LDFLAGS}
    ${CC} ${S}/ampere_fru_upgrade.c -o ${S}/ampere_fru_upgrade -I${ROOT}/usr/include/ ${LDFLAGS}
    ${CC} ${S}/ampere_flashcp.c -o ${S}/ampere_flashcp -I${ROOT}/usr/include/ ${LDFLAGS}
}

do_install_append() {
    install -d ${D}/usr/sbin
    install -m 0755 ${S}/ampere_eeprom_prog ${D}/${sbindir}/ampere_eeprom_prog
    install -m 0755 ${S}/nvparm ${D}/${sbindir}/nvparm
    install -m 0755 ${WORKDIR}/ampere_firmware_upgrade.sh ${D}/${sbindir}/ampere_firmware_upgrade.sh
    install -m 0755 ${WORKDIR}/ampere_flash_bios.sh ${D}/${sbindir}/ampere_flash_bios.sh
    install -m 0755 ${S}/ampere_fru_upgrade ${D}/${sbindir}/ampere_fru_upgrade
    install -m 0755 ${S}/ampere_flashcp ${D}/${sbindir}/ampere_flashcp
}
