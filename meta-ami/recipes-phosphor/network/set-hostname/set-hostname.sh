#!/bin/sh -eu

sync_hostname() {

    MAC=$(fw_printenv | grep eth1addr | cut -d'=' -f2)
    hostnamectl set-hostname AMI${MAC}
}

if [ "$(hostname)" == "{MACHINE}" ];then
        sync_hostname
fi

