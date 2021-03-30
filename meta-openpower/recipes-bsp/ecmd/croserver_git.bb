SUMMARY = "eCMD"
DESCRIPTION = "eCMD is a hardware access API for IBM Systems"
LICENSE= "Apache-2.0"
LIC_FILES_CHKSUM = "file://${S}/NOTICE;md5=fee220301a2af3faf8f211524b4248ea"

SRC_URI = "git://github.com/open-power/eCMD.git"
SRCREV = "79cadcd0d299cc044791da9b5e3746108290558a"
DEPENDS += "zlib"

SRC_URI += "file://croserver.service"

S = "${WORKDIR}/git"

inherit python3native
DEPENDS += "${PYTHON_PN}-distro-native"

do_configure() {
   LD="${CXX}" ${PYTHON} ${S}/config.py \
       --without-swig --output-root ${B} --target obj \
       --extensions "cmd cip" --build-verbose
}

do_compile() {
    cd ${S}/dllNetwork/server
    oe_runmake
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 out_obj/lib/server1p ${D}${bindir}/croserver

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/croserver.service ${D}${systemd_system_unitdir}/
}

FILES_${PN} += "${systemd_system_unitdir}/croserver.service"
