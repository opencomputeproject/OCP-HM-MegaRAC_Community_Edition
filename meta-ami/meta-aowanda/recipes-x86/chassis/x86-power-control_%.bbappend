FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"

SRC_URI_append += " file://0001-json-file-modified-for-aowanda_shortName_Rst.patch"
SRC_URI_append += " file://0002-restorepowerpolicy-support.patch"
SRC_URI_append += " file://0004-Update-service-file.patch"
SRC_URI_append += " file://0005-aowanda_host_power_operation_changes_modified.patch"
