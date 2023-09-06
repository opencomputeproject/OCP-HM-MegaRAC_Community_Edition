#!/bin/sh

for i in /etc/dns.d/nsupdate_tmp-del*; do
    if ! [ -f $i ]; then
        continue
    fi

    nsupdate "$i" &
    echo "$i"
    COUNT=0
    while [ $COUNT != 3 ];
    do
        COUNT=$(($COUNT+1))
        ps | grep -v grep | grep -q "nsupdate $i"
        if [ $? != 0 ]; then
            break
        fi
        sleep 1
    done

    ps | grep "nsupdate $i" | grep -v grep| awk '{print $1}' | xargs kill > /dev/null 2>&1
done

ENABLED=`busctl get-property xyz.openbmc_project.Network /xyz/openbmc_project/network/dhcp xyz.openbmc_project.Network.DHCPConfiguration SendNsupdateEnabled | cut -d" " -f2`
if [ "$ENABLED" = "true" ]; then

    for i in /etc/dns.d/nsupdate_tmp-add*; do
        if ! [ -f $i ]; then
            continue
        fi

        nsupdate "$i" &
        echo "$i"
        COUNT=0
        while [ $COUNT != 3 ];
        do
            COUNT=$(($COUNT+1))
            ps | grep -v grep | grep -q "nsupdate $i"
            if [ $? != 0 ]; then
                break
            fi

            sleep 1
        done

        ps | grep "nsupdate $i" | grep -v grep| awk '{print $1}' | xargs kill > /dev/null 2>&1
    done
fi

busctl set-property xyz.openbmc_project.Network /xyz/openbmc_project/network/dns xyz.openbmc_project.Network.DDNS SetInProgress b false
