LINUX_VERSION = "5.4.53"
require linux-aspeed.inc

addtask cpy_kernel_src before do_kernel_checkout after do_unpack

# Copy kernel source code to ${S} which is needed by kernel checkout function
do_cpy_kernel_src() {
        cp -r ${TOPDIR}/../openbmc_modules/linux/* ${S}/
}

