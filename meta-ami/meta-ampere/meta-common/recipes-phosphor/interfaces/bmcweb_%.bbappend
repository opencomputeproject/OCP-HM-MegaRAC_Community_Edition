FILESEXTRAPATHS_append := "${THISDIR}/${PN}:"
# SRCREV = "ffed87b5ad1797ca966d030e7f97977028d258fa"

# EXTRA_OEMESON_append = " \
#      -Dinsecure-tftp-update=disabled \
#      -Dbmcweb-logging=enabled \
#      -Dredfish-bmc-journal=enabled \
#      -Dvm-nbdproxy=enabled \
#      "

SRC_URI_append += " \
            file://0001-Redfish-Add-message-registries-for-Ampere-event.patch \
           "
# file://0002-Re-enable-vm-nbdproxy-for-Virtual-Media.patch
# file://0003-Fix-build-issue-when-virtual-media-is-enabled.patch
# file://0004-power-control-Fix-power-cycle-issue.patch
