SUMMARY = "PWMTACH test application"
SECTION = "apps"
LICENSE = "CLOSED"

APP_NAME = "pwmtachtool"
bindir = "/usr/bin"

SRC_URI = "git://github.com/openbmc/openbmc-tools;protocol=https;branch=master"
SRC_URI += "file://0002-pwm-max-min-value-range.patch"
SRC_URI += "file://0003-set-fan-speed-set-dutycycle.patch"
SRC_URI += "file://0001-Mapping-pwm-tach-number-starting-from-0.patch"

SRCREV = "90cb34fc2f169aef4bb740c95daefcd8f16e0353"

S = "${WORKDIR}/git/hongweiz/pwmtachtool/src"
PV = "0.1+git${SRCPV}"

do_compile() {
     ${CC} pwmtachtool.c pwmtach.c  EINTR_wrappers.c ${LDFLAGS} -o pwmtachtool

}

do_install () {
    install -d ${D}${bindir}
    install -m 0755 -d ${D}${bindir}
    cd ${S}
    install -m 0755 ${APP_NAME} ${D}${bindir}
}

FILES_${PN}-dev = ""
FILES_${PN} = "${bindir}/*"
