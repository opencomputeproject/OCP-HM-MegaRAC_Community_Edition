#!/bin/bash

source /usr/sbin/gpio-lib.sh

# GPIOAC3 BMC_SPI0_BACKUP_SEL Boot from MAIN SPI-HOST
# FIXME: GPIO227 is always set to HIGH after BMC boot, even it is
# set to LOW in u-boot
gpio_configure_output 227 0

