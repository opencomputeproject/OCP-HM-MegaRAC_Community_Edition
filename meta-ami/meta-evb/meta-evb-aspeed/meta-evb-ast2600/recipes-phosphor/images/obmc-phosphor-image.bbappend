FILESEXTRAPATHS:append:= "${THISDIR}/files:"

INITRAMFS_ROOTFS = "${TMPDIR}/work/${MACHINE}-openbmc-${TARGET_OS}/${INITRAMFS_IMAGE}/${PV}-${PR}/rootfs"

do_generate_static:append() {

    verity_image = d.getVar('DM_VERITY_IMAGE', True)
    if verity_image:
        _append_image(os.path.join(d.getVar('IMGDEPLOYDIR', True),
                                '%s.%s.verity' % (
                                    d.getVar('IMAGE_LINK_NAME', True),
                                    d.getVar('IMAGE_BASETYPE', True))),
                    int(d.getVar('FLASH_ROFS_OFFSET', True)),
                    int(d.getVar('FLASH_RWFS_OFFSET', True)))

    if not verity_image:
        _append_image(os.path.join(d.getVar('IMGDEPLOYDIR', True),
                                '%s.%s' % (
                                    d.getVar('IMAGE_LINK_NAME', True),
                                    d.getVar('IMAGE_BASETYPE', True))),
                    int(d.getVar('FLASH_ROFS_OFFSET', True)),
                    int(d.getVar('FLASH_RWFS_OFFSET', True)))
}

do_mk_static_symlinks:append() {
    if [ -f ${IMAGE_LINK_NAME}.${IMAGE_BASETYPE}.verity ]
    then
        ln -sf ${IMAGE_LINK_NAME}.${IMAGE_BASETYPE}.verity ${IMGDEPLOYDIR}/image-rofs
    else
        ln -sf ${IMAGE_LINK_NAME}.${IMAGE_BASETYPE} ${IMGDEPLOYDIR}/image-rofs
    fi
}

#have to run POSTPROCESS when verity enabled
deploy_verity_hash() {
    install -D -m 0755 \
        ${STAGING_VERITY_DIR}/${DM_VERITY_IMAGE}.${DM_VERITY_IMAGE_TYPE}.verity.env \
        ${INITRAMFS_ROOTFS}/dm-verity.env
}
IMAGE_POSTPROCESS_COMMAND += '${@bb.utils.contains("DM_VERITY_IMAGE", "obmc-phosphor-image", "deploy_verity_hash; ", "",d)}'

OBMC_IMAGE_EXTRA_INSTALL:append = " \
                                  ipmitool \
                                  phosphor-sel-logger \
                                  phosphor-host-postd \
                                  phosphor-post-code-manager \
                                  entity-manager \
                                 "
