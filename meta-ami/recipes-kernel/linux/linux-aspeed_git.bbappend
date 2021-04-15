FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0002-Backend-slave-driver-ipmb-mqueue-for-slave-read-drivers_i2c_i2c-slave-read.c.patch"
SRC_URI += "${@ "file://0009-Enable-AMI-extend-IOCTLS-and-Slave-function.patch" if bb.utils.to_boolean(d.getVar('APPLY_EXTRA_PATCHES')) else ""}"
SRC_URI += "file://0010-mm_bigphysarea.patch"
SRC_URI += "file://0014-flash-layout-v2019-uboot.patch"
SRC_URI += "file://0015-drivers_net_ethernet_faraday_ftgmac100.h.patch"
SRC_URI += "file://0016-kernel-source-drivers-net-ethernet-faraday-ftgmac100.c.patch"
SRC_URI += "file://0018-drivers_net_ethernet_faraday_ftgmac100.patch"
SRC_URI += "file://0022-dma-mapping-retive-NULL-device-support.patch"
SRC_URI += "file://kernel.cfg"

#OSP2.0
SRC_URI += "file://047-enable-usb-ether.patch"
SRC_URI += "file://056-drivers_usb_gadget_function_coreTypes_h.patch"
SRC_URI += "file://057-drivers_usb_gadget_function_f_mass_storage_c.patch"
SRC_URI += "file://058-drivers_usb_gadget_function_f_mass_storage_h.patch"
SRC_URI += "file://059-drivers_usb_gadget_ami_gadget_ioctl_h.patch"
SRC_URI += "file://060-mt25ql01gb-spi-support.patch"

