FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append += "file://0001-unlock-user-when-reach-the-timeout.patch"

RDEPENDS_${PN} += "nss-pam-ldapd phosphor-ldap"
