FILESEXTRAPATHS_prepend_mtjade := "${THISDIR}/${PN}:"

SRC_URI += " \
	file://0001-Make-snoop-port-optional-for-the-daemon.patch  \
	"

SNOOP_DEVICE_mtjade = ""
