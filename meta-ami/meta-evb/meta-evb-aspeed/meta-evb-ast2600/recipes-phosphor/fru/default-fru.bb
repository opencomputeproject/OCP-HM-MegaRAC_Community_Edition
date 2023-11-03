SUMMARY = "Default Fru"
DESCRIPTION = "Installs a default fru file to image"


S = "${WORKDIR}"
SRC_URI = "file://baseboard.fru.bin"

LICENSE = "CLOSED"


do_install() {

    install -d ${D}${sysconfdir}/fru
    cp ${S}/baseboard.fru.bin ${D}/${sysconfdir}/fru

}
