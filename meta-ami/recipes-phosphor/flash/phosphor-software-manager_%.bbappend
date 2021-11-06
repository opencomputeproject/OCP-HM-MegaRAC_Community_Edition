FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append += "file://0001-avoid-same-version-fw-upload.patch \
                   file://0002-add-signature-verification-for-image-bmc.patch \
		   file://0003-full-image-support.patch \
		   file://0004-proper-error-fw-failure.patch \
		  " 

PACKAGECONFIG_append = " verify_signature"

