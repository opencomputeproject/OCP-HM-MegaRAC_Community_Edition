#!/bin/bash

flag=1
while [ $flag -eq 1 ]
do
ipmi=$(ps | grep -w /usr/bin/netipmid | wc | awk '{print $1}')
kcs=$(ps | grep -w /usr/bin/kcsbridged | wc | awk '{print $1}')
if [ $ipmi -gt 1 -a $kcs -gt 1 ]; then
	sleep 2
	/usr/bin/gpioset gpiochip0 200=1
	sleep 2
        host_status=`/usr/bin/gpioget gpiochip0 174`
        if [[ $host_status -eq 0 ]]
        then
                /usr/bin/gpioset gpiochip0 40=1
                /usr/bin/gpioset gpiochip0 43=1
                busctl set-property "xyz.openbmc_project.State.Host" "/xyz/openbmc_project/state/host0" "xyz.openbmc_project.State.Host" RequestedHostTransition s "xyz.openbmc_project.State.Host.Transition.On"
        fi

flag=0
fi
sleep 1
done

exit 0
