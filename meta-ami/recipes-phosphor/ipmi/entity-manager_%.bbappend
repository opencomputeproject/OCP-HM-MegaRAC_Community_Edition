FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append += "file://0001-Added-SdrRecord-config.patch"
SRC_URI_append += "file://0002-Updated-SDR-configuration.patch"
#SRC_URI_append += "file://0003-gpiosensor-export-template.patch"
#SRC_URI_append += "file://0004-dualgpio-export-template.patch"
SRC_URI_append += "file://0005-fixed-fru-size-issue.patch"
SRC_URI_append += "file://0006-backup-thresholds-to-different-file.patch"

