FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "601d3db44097d89a7d24ae287e8bbe72e7e92d5d"


EXTRA_OECONF += "${@bb.utils.contains_any("IMAGE_FEATURES", [ 'debug-tweaks', 'allow-root-login' ], '', '--disable-root_user_mgmt', d)}"

SRC_URI += " \
	     file://0003-Add-Host-Interface-User-Support.patch \
	     file://0012-passwordpolicy.patch \
           "

FILES:${PN} += "${datadir}/dbus-1/system.d/phosphor-nslcd-cert-config.conf"
FILES:${PN} += "/usr/share/phosphor-certificate-manager/nslcd"
FILES:${PN} += "\
    /lib/systemd/system/multi-user.target.wants/phosphor-certificate-manager@nslcd.service"

#file://0015-passwordchangerequired.patch 
