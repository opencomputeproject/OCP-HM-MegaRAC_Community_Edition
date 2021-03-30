#KBRANCH ?= "dev-5.4"
LINUX_VERSION ?= "5.4.53"

#SRCREV="a17b8ac585d7faa27799f425fa4326c7a1e7ae71"

require linux-nuvoton.inc

addtask cpy_kernel_src before do_kernel_checkout after do_unpack

# Copy kernel source code to ${S} which is needed by kernel checkout function
do_cpy_kernel_src() {
        cp -r ${TOPDIR}/../openbmc_modules/linux/* ${S}/
}
