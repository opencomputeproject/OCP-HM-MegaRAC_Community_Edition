#!/bin/sh
#Trigger the host power status monitor

SERVICE="xyz.openbmc_project.State.Host"

HOST_OBJECT="/xyz/openbmc_project/state/host0"
OS_OBJECT="/xyz/openbmc_project/state/os"

HOST_INTERFACE="xyz.openbmc_project.State.Host"
OS_INTERFACE="xyz.openbmc_project.State.OperatingSystem.Status"

host_status=`gpioget 0 174`

sleep 5

host_status=`gpioget 0 174`

dbus_power_status=`busctl get-property $SERVICE $HOST_OBJECT $HOST_INTERFACE RequestedHostTransition`
dbus_os_status=`busctl get-property $SERVICE $OS_OBJECT $OS_INTERFACE OperatingSystemState`

if [[ $host_status -eq  0  &&  $dbus_os_status == "s \"Standby\"" && $dbus_power_status != "s \"xyz.openbmc_project.State.Host.Transition.Off\"" ]]
then
	echo "host powering off"
	busctl set-property $SERVICE $HOST_OBJECT $HOST_INTERFACE RequestedHostTransition s "xyz.openbmc_project.State.Host.Transition.Off"

elif [[ $host_status -eq  1  &&  $dbus_os_status != "s \"Standby\"" && $dbus_power_status != "s \"xyz.openbmc_project.State.Host.Transition.On\"" ]]
then
	echo "host powering on"
	busctl set-property $SERVICE $HOST_OBJECT $HOST_INTERFACE RequestedHostTransition s "xyz.openbmc_project.State.Host.Transition.On"

fi
