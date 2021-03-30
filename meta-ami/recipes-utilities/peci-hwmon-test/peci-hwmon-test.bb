SUMMARY = "PECI hwmon test tool"
DESCRIPTION = "command line python tool for testing PECI hwmon"

SRC_URI = "\
    file://peci-hwmon-test.py \
    "
LICENSE = "CLOSED"

DEPENDS += "python3"
RDEPENDS_${PN} += "python3"

INSANE_SKIP_${PN} = " file-rdeps "

S = "${WORKDIR}"

do_compile () {
}

do_install () {
    install -d ${D}/${bindir}
    install -m 0755 ${WORKDIR}/peci-hwmon-test.py ${D}/${bindir}
}

