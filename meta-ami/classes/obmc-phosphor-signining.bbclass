
SIGNING_KEY ?= "${STAGING_DIR_NATIVE}${datadir}/OpenBMC.priv"
INSECURE_KEY = "${@'${SIGNING_KEY}' == '${STAGING_DIR_NATIVE}${datadir}/OpenBMC.priv'}"
SIGNING_KEY_DEPENDS = "${@oe.utils.conditional('INSECURE_KEY', 'True', 'phosphor-insecure-signing-key-native:do_populate_sysroot', '', d)}"



make_signatures() {
	signature_files=""
        cd "${present_directory}"
	for file in "$@"; do
                pwd
                echo "openssl dgst -sha256 -sign ${SIGNING_KEY} -out "${file}.sig" $file"
		openssl dgst -sha256 -sign ${SIGNING_KEY} -out "${file}.sig" $file
		signature_files="${signature_files} ${file}.sig"
	done

	if [ -n "$signature_files" ]; then
		sort_signature_files=`echo "$signature_files" | tr ' ' '\n' | sort | tr '\n' ' '`
		cat $sort_signature_files > image-full
		openssl dgst -sha256 -sign ${SIGNING_KEY} -out image-full.sig image-full
		signature_files="${signature_files} image-full.sig"
	fi
}

do_generate_full_image_tar() {
    cd "${B}/imgfull"
    present_directory="${B}/imgfull"
    # add symlinks for the contents
    ln -sf "${DEPLOY_DIR_IMAGE}/image-mtd" "image-bmc"
    ln -sf "${B}/publickey" .
    ln -sf ${B}/MANIFEST .
    make_signatures image-bmc MANIFEST publickey
    tar -h -cvf "${DEPLOY_DIR_IMAGE}/${PN}-image-update-full-${MACHINE}-${DATETIME}.tar" image-bmc MANIFEST publickey ${signature_files}
    # make a symlink
    ln -sf "${PN}-image-update-full-${MACHINE}-${DATETIME}.tar" "${DEPLOY_DIR_IMAGE}/image-update-full-${MACHINE}"
    ln -sf "${PN}-image-update-full-${MACHINE}-${DATETIME}.tar" "${DEPLOY_DIR_IMAGE}/OBMC-full-${@ do_get_version(d)}-oob.bin"
    ln -sf "image-update-full-${MACHINE}" "${DEPLOY_DIR_IMAGE}/image-update-full"
    ln -sf "image-update-full-${MACHINE}" "${DEPLOY_DIR_IMAGE}/OBMC-full-${@ do_get_version(d)}-inband.bin"

}

do_generate_full_image_tar[vardepsexclude] = "DATETIME"
do_generate_full_image_tar[dirs] = "${S}/imgfull"
do_generate_full_image_tar[depends] += " \
        openssl-native:do_populate_sysroot \
        ${SIGNING_KEY_DEPENDS} \
        ${PN}:do_copy_signing_pubkey \
        "

do_image_signed_fitimage_rootfs() {
    cd "${B}/img"
    ln -sf "${B}/publickey" . 
    make_signatures image-kernel image-rofs image-rwfs image-u-boot MANIFEST publickey
    tar -h -cvf "${DEPLOY_DIR_IMAGE}/${PN}-image-update-${MACHINE}-${DATETIME}.tar" MANIFEST image-u-boot image-runtime image-kernel image-rofs image-rwfs publickey ${signature_files}
    ln -sf "${PN}-image-update-${MACHINE}-${DATETIME}.tar" "${DEPLOY_DIR_IMAGE}/image-update-${MACHINE}"
    ln -sf "${PN}-image-update-${MACHINE}-${DATETIME}.tar" "${DEPLOY_DIR_IMAGE}/OBMC-${@ do_get_version(d)}-oob.bin"
    ln -sf "image-update-${MACHINE}" "${DEPLOY_DIR_IMAGE}/image-update"
    ln -sf "image-update-${MACHINE}" "${DEPLOY_DIR_IMAGE}/OBMC-${@ do_get_version(d)}-inband.bin"
}

do_image_signed_fitimage_rootfs[vardepsexclude] = "DATETIME"
do_image_signed_fitimage_rootfs[dirs] = "${S}"
do_image_signed_fitimage_rootfs[depends] += " \
        openssl-native:do_populate_sysroot \
        ${SIGNING_KEY_DEPENDS} \
        ${PN}:do_copy_signing_pubkey \
        "


python() {

        bb.build.addtask(
                'do_generate_full_image_tar',
                'do_build',
                ' do_copy_signing_pubkey do_generate_auto ', d)
        
        bb.build.addtask(
                'do_image_signed_fitimage_rootfs',
                'do_build',
                ' do_image_fitimage_rootfs ', d)
}
