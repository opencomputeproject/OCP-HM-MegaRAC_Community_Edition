FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append += "file://0001-same-version-fw-should-not-allow-to-upload.patch \
                   file://0002-static-mtd-all-tar-image-support.patch \
		   file://0003-show-fw-version-after-activate-image.patch \
                   file://0004-mtd-all-tar-image-signature-verification.patch \
                   file://0005-error-log-on-image-fail-to-upload.patch \
		  " 

PACKAGECONFIG_append = " verify_signature"
