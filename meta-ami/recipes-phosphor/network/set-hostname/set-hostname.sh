#!/bin/sh -eu

sync_hostname() {

    MAC=$(cat /sys/class/net/eth0/address)
    hostnamectl set-hostname AMI${MAC}
}

if [ "$(hostname)" == "{MACHINE}" ];then
        sync_hostname
fi

