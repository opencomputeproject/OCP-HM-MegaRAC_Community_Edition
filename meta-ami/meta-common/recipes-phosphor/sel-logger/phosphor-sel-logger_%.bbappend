FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
PROJECT_SRC_DIR := "${THISDIR}/${PN}"

SRCREV = "7c2810b482786ab2d05cf81633d6abb6ec577212"

SRC_URI += "\
    file://0007-add-pef.patch \
    file://0008-linear_sel_policy_support.patch \
"

DEPENDS += "intel-ipmi-oem"
RDEPENDS:${PN} += "intel-ipmi-oem"


PACKAGECONFIG:append:intel = " clears-sel"

PACKAGECONFIG:append:intel = " log-watchdog"

