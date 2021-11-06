FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
inherit devnet

SRC_URI_append += "file://0001-secureLdap-allow-selfsigned.patch"

EXTRA_OECONF_append = " \
${@get_devnet_feature('OBMC_FEATURE_DISABLE_DEFAULT_LOGIN', '--disable-root_user_mgmt', '', d)}"
