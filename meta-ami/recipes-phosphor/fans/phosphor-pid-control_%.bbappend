FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

EXTRA_OECONF +=  "--enable-configure-dbus=yes"
