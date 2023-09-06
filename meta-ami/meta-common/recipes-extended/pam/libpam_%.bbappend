RDEPENDS:${PN}-runtime += "${MLPREFIX}pam-plugin-localuser-${libpam_suffix}"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-pam-cracklib-passwordpolicy-update.patch \
            file://common-authentication \
            "


#Default settings lockout duration to 300 seconds and threshold value to 10
do_install:append() {
   rm -rf  ${D}${sysconfdir}/pam.d/common-auth
   cp ${WORKDIR}/common-authentication ${D}${sysconfdir}/pam.d/common-auth
}
