FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-add-to-warm-reset.patch \
            file://0002-get-channel-authentication-return-value-fix.patch \
	    file://0003-callback-privilege-fix.patch \
            file://0004-cipher-suite-privilege-fix.patch \
            file://0005-cipher-suite-oem-callback-fix.patch"
