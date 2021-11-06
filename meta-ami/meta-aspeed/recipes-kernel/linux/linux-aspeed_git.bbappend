FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:${TOPDIR}/../kernel/packages/Kernel_5_ext-src/data/:"

require ${AMIBASE_KERNEL}/kernel-patch.inc

#SPX-13
SRC_URI += "file://011-linux_arch_arm_boot_dts_aspeed-g6-pinctrl_dtsi.patch"
SRC_URI += "file://012-linux-new_arch_arm_mach-aspeed_platsmp_c.patch"
SRC_URI += "file://014-linux_include_dt-bindings_clock_ast2600-clock_h.patch"
SRC_URI += "file://015-linux_drivers_clk_clk-ast2600_c.patch"
SRC_URI += "file://016-linux_drivers_clocksource_timer-fttmr010_c.patch"
SRC_URI += "file://017-linux_driver_fsi.patch"
SRC_URI += "file://018-driver_gpio_sgpio.patch"
SRC_URI += "file://019-linux_drivers_hwmon_aspeed2600-pwm-tacho.patch"
SRC_URI += "file://021-linux_drivers_iio_adc_aspeed_adc.patch"
SRC_URI += "file://022-linux_drivers_irqchip.patch"
#SRC_URI += "file://023-linux_drivers_soc_aspeed_espi.patch"
SRC_URI += "file://024-linux_drivers_mtd_spi-nor_aspeed-smc_c.patch"
SRC_URI += "file://027-linux_drivers_pinctrl_aspeed.patch"
SRC_URI += "file://028-linux_drivers_reset.patch"
SRC_URI += "file://029-linux_drivers_soc_aspeed.patch"
SRC_URI += "file://033-linux_drivers_watchdog.patch"
SRC_URI += "file://036-linux_drivers_mctp.patch"
SRC_URI += "file://037-linux_drivers_jtag.patch"
#SRC_URI += "file://037-linux_drivers_usb_gadget_udc_aspeed-vhub.patch"
SRC_URI += "file://037-linux_drivers_usb_gadget_udc.patch"
SRC_URI += "file://039-drivers_i2c_busses_ast2600-i2c-global_c.patch"
SRC_URI += "file://040-drivers_i2c_busses_ast2600-i2c-global_h.patch"
SRC_URI += "file://042-drivers_i2c_busses_i2c-new-aspeed.patch"
SRC_URI += "file://043-drivers_i2c_busses_Kconfig.patch"
SRC_URI += "file://045-drivers_mmc.patch"
SRC_URI += "file://047-linux_drivers_char_ipmi_kcs_bmc_aspeed_c.patch"
SRC_URI += "file://048-linux_drivers_soc_aspeed_aspeed-lpc-snoop_c.patch"
#SRC_URI += "file://050-drivers_usb_gadget_udc_aspeed-vhub_epn_c.patch"
SRC_URI += "file://051-linux_driver_mtd_spi-nor_spi-nor_c.patch"

#OSP2.1
SRC_URI += "file://063-KVM-AST-2600-Support.patch"

#OSP2.2
SRC_URI += "file://kernel_aspeed.cfg"
SRC_URI += "file://0001-aspeed-g6-ami.dtsi.patch"
SRC_URI += "file://0002-aspeed-ast2600-evb-ami.dts.patch"
SRC_URI += "file://0003-drivers_i2c_busses_Makefile.patch"
SRC_URI += "file://0004-aspeed-espi.patch"
SRC_URI += "file://0005-drivers-hwmon-aspeed2600-pwm-tacho.c.patch"
SRC_URI += "file://0004-updated-aspeed-ast2600-evb-ami.dts.patch"
SRC_URI += "file://kvm_aspeed.cfg"
SRC_URI += "file://068-jtag-driver.patch"
SRC_URI += "file://069-redirect-windows-ISO-to-CD-support-large-size-images.patch"
SRC_URI += "file://070-justified-removal-endpoint-request-error.patch"
SRC_URI += "file://071-FixResetIssueInNewI2CDriver.patch"
SRC_URI += "file://072-re-enable-passthru-mux.patch"

#SPX-13
SRC_URI += "file://064-8250_aspeed_uart.patch"

