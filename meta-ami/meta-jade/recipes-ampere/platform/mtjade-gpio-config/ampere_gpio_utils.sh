#!/bin/bash
# Usage of this utility
function set_gpio_active_low() {
    if [ $# -ne 2 ]; then
        echo "set_gpio_active_low: need both GPIO# and initial level";
        return;
    fi

    if [ ! -d /sys/class/gpio/gpio$1 ]; then
        echo $1 > /sys/class/gpio/export
    fi
    echo $2 > /sys/class/gpio/gpio$1/direction

    if [ -d /sys/class/gpio/gpio$1 ]; then
        echo $1 > /sys/class/gpio/unexport
    fi
}

GPIO_OCP_AUX_PWREN=139
GPIO_OCP_MAIN_PWREN=140
GPIO_SPI0_PROGRAM_SEL=226
GPIO_SPI0_BACKUP_SEL=227

GPIO_BASE=$(cat /sys/class/gpio/gpio*/base)

function usage() {
    echo "usage: ampere-util [power] [on|off]";
}

set_gpio_power_off() {
    echo "Setting GPIO before Power off"
    set_gpio_active_low $((${GPIO_BASE} + ${GPIO_OCP_AUX_PWREN})) high
    set_gpio_active_low $((${GPIO_BASE} + ${GPIO_OCP_MAIN_PWREN})) low
    set_gpio_active_low $((${GPIO_BASE} + ${GPIO_SPI0_PROGRAM_SEL})) low
}

set_gpio_power_on() {
    echo "Setting GPIO before Power on"
    set_gpio_active_low $((${GPIO_BASE} + ${GPIO_OCP_AUX_PWREN})) high
    set_gpio_active_low $((${GPIO_BASE} + ${GPIO_OCP_MAIN_PWREN})) high
    set_gpio_active_low $((${GPIO_BASE} + ${GPIO_SPI0_PROGRAM_SEL})) high
    set_gpio_active_low $((${GPIO_BASE} + ${GPIO_SPI0_BACKUP_SEL})) low
}

if [ $# -lt 2 ]; then
    echo "Total number of parameter=$#"
    echo "Insufficient parameter"
    usage;
    exit 0;
fi

if [ $1 == "power" ]; then
    if [ $2 == "on" ]; then
        set_gpio_power_on
    elif [ $2 == "off" ]; then
        set_gpio_power_off
    fi
    exit 0;
else
    echo "Invalid parameter1=$1"
    usage;
    exit 0;
fi
exit 0;
