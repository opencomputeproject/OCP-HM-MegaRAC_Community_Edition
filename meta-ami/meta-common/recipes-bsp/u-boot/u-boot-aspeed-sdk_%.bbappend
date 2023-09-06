FILESEXTRAPATHS:append:= "${THISDIR}/files:"

SRC_URI:append = " \
    file://ast2600_a3.json \
    "

EVB_SRC_URI = " file://spl.cfg"
AC_SRC_URI = " file://spl_archercity.cfg"
NO_SPL_URI = " file://nospl.cfg"

SRC_URI:append:evb-ast2600 = "${@bb.utils.contains('SPL_BINARY', 'spl/u-boot-spl.bin', EVB_SRC_URI, '', d)}"
SRC_URI:append:intel-ast2600 = "${@bb.utils.contains('SPL_BINARY', 'spl/u-boot-spl.bin', AC_SRC_URI, NO_SPL_URI, d)}"

#Enable ASPEED SOC Secure Boot
SOCSEC_SIGN_ENABLE = "0"

SOCSEC_SIGN_KEY = "${WORKDIR}/keys/SIG_RSA_KEY2_private.pem"
SOCSEC_SIGN_ALGO = "RSA2048_SHA256"
OTPTOOL_CONFIGS = "${WORKDIR}/ast2600_a3.json"
OTPTOOL_KEY_DIR = "${WORKDIR}/keys/"

SOCSEC_SIGN_EXTRA_OPTS = "--rsa_key_order=little"

do_deploy:prepend() {
        # otptool needs access to the public and private socsec signing keys in the keys/ directory. uncomment if SOCSEC enabled
        # openssl rsa -in ${SOCSEC_SIGN_KEY} -pubout > ${WORKDIR}/keys/SIG_RSA_KEY2_public.pem
}

SRC_URI_NON_PFR = "file://0001-adding-Fieldmode-to-enable-failure-when-signature-va.patch"
SRC_URI:append = " ${@bb.utils.contains('IMAGE_FSTYPES', 'intel-pfr', '', SRC_URI_NON_PFR, d)}"

SRC_URI:append:intel-ast2600 = "${@bb.utils.contains('IMAGE_FSTYPES', 'intel-pfr', '', ' file://flash-layout-update.cfg  ', d)}"
