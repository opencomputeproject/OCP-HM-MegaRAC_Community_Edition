# Recipe created by recipetool
# This is the basis of a recipe and may need further editing in order to be fully functional.
# (Feel free to remove these comments when editing.)

# WARNING: the following LICENSE and LIC_FILES_CHKSUM values are best guesses - it is
# your responsibility to verify that the values are complete and correct.
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d6c07c78b4389c35f5005aabfce73863"

SRC_URI = "git://github.com/JoeKuan/Network-Interfaces-Script;protocol=https \
           file://0001-Enable-script-executable.patch \
           "
RDEPENDS_${PN} = " gawk"

# Modify these as desired
PV = "1.0+git${SRCPV}"
SRCREV = "ecec9e569fe4c1a4332b3a0d027a00887d9c2d1d"


PACKAGES = "${PN} ${PN}-dev"

S = "${WORKDIR}/git"

# NOTE: no Makefile found, unable to determine what needs to be done

do_configure () {
	# Specify any needed configure commands here
	:
}

do_compile () {
	# Specify compilation commands here
	:
}

do_install () {
    install -d ${D}${sysconfdir}
    install -m 755 *.awk ${D}${sysconfdir}
}

FILES_${PN}-dev += "${sysconfdir}/*"
FILES_${PN} += "${sysconfdir}/*"