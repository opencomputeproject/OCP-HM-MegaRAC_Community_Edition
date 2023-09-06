inherit obmc-phosphor-signining
python() {
    types = d.getVar('IMAGE_FSTYPES', True).split()
    d.setVar('UBOOT_SEC_SIZE', str(1024*1024))

    if not 'intel-pfr' in types:
        d.setVar('FIT_SECTOR_SIZE', str(0x1f00000))
        DTB_FULL_FIT_IMAGE_OFFSETS = [0x100000]
        d.setVar('FLASH_RUNTIME_OFFSETS', ' '.join(
            [str(int(x/1024)) for x in DTB_FULL_FIT_IMAGE_OFFSETS])
            )

}
