FILESEXTRAPATHS_append_mtjade := "${THISDIR}/${PN}:"

EXTRA_OECONF_append_mtjade = " --enable-nic-ethtool=yes"
