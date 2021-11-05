#!/bin/bash
# Initialize variables
boot_stage=0x00
boot_status=0x00
uefi_code=0x00000000

function set_postcode()
{
	postcode=$( printf "0x%02x%02x%08x" $1 $2 $3 )
	busctl set-property xyz.openbmc_project.State.Boot.Raw \
		/xyz/openbmc_project/state/boot/raw \
		xyz.openbmc_project.State.Boot.Raw Value t $postcode
}

function update_boot_progress()
{
	bootprog=$1

	busctl set-property xyz.openbmc_project.State.Host \
		/xyz/openbmc_project/state/host0 \
		xyz.openbmc_project.State.Boot.Progress \
		BootProgress s \
		xyz.openbmc_project.State.Boot.Progress.ProgressStages.$bootprog
}

function get_boot_stage_string()
{
	bootstage=$1
	ueficode=$2

	case $bootstage in

		0x00)
			boot_stage_str="SMpro"
			;;

		0x01)
			boot_stage_str="PMpro"
			;;

		0x02)
			boot_stage_str="ATF BL1 (Code=${ueficode})"
			;;

		0x03)
			boot_stage_str="DDR initialization (Code=${ueficode})"
			;;

		0x04)
			boot_stage_str="DDR training progress (Code=${ueficode})"
			;;

		0x05)
			boot_stage_str="ATF BL2 (Code=${ueficode})"
			;;

		0x06)
			boot_stage_str="ATF BL31 (Code=${ueficode})"
			;;

		0x07)
			boot_stage_str="ATF BL32 (Code=${ueficode})"
			;;

		0x08)
			boot_stage_str="UEFI booting (UEFI Code=${ueficode})"
			;;

	esac

	echo $boot_stage_str
}

function set_boot_progress()
{
	boot_stage=$1
	uefi_code=$2

	case $boot_stage in

		0x02)
			update_boot_progress "PrimaryProcInit"
			;;

		0x03)
			update_boot_progress "MemoryInit"
			;;

		0x08)
			if [[ "$uefi_code" =~ 0x0201* ]]; then
				update_boot_progress "PCIInit"
			elif [[ "$uefi_code" =~ 0x0202* ]]; then
				update_boot_progress "SystemInitComplete"
			fi
			;;

	esac
}

function log_redfish_biosboot_ok_event()
{
	logger-systemd --journald << EOF
MESSAGE=
PRIORITY=2
SEVERITY=
REDFISH_MESSAGE_ID=OpenBMC.0.1.BIOSBoot.OK
REDFISH_MESSAGE_ARGS="UEFI firmware booting done"
EOF
}

function log_redfish_bios_panic_event()
{
	boot_state_str=$(get_boot_stage_string $1 $2)

	logger-systemd --journald << EOF
MESSAGE=
PRIORITY=2
SEVERITY=
REDFISH_MESSAGE_ID=OpenBMC.0.1.BIOSFirmwarePanicReason.Warning
REDFISH_MESSAGE_ARGS=${boot_state_str}
EOF
}

# Check if the Host is already booted or not. If the Host is already booted,
# do not log event to avoid redundant BIOSBoot.OK event
host_booted=1
cnt=0

while [ $cnt -lt 100 ];
do
	# Sleep 200ms
	usleep 200000
	bg=(`cat /sys/bus/i2c/devices/2-004f/1e78a0c0.i2c-bus:smpro@4f:misc/boot_progress`)
	if [ $? -ne 0 ]; then
		cnt=$((cnt + 1))
		# When boot-progress is running but suddenly off or reboot,
		# the /sys interface is unavailable. Stop executing the script
		if [ ${host_booted} == "0" ];
		then
			break
		else
			sleep 1
			continue
		fi
	fi
	cnt=0

	# Check if any update from previous check
	if ([ "${boot_stage}" == "${bg[0]}" ] && [ "${boot_status}" == "${bg[1]}" ] && [ "${uefi_code}" == "${bg[2]}" ]); then
		continue
	fi

	# Check if the Host is already ON or not. If Host is already boot, update boot progress and break.
	if ([ ${boot_stage} == "0x00" ] && [ ${bg[0]} == "0x08" ]);
	then
		update_boot_progress "OSStart"
		break
	fi
	host_booted=0

	# Update current boot progress
	boot_stage=${bg[0]}
	boot_status=${bg[1]}
	uefi_code=${bg[2]}
	echo "Boot Progress = ${boot_stage} ${boot_status} ${uefi_code}"

	# Log Boot Progress to dbus
	if [ ${boot_status} == "0x03" ]; then
		# Log Redfish Event if failure.
		log_redfish_bios_panic_event $boot_stage $uefi_code
		# Dimm training failed, check errors
		if [ ${boot_stage} == "0x04" ]; then
			/usr/sbin/dimm_train_fail_log.sh 0
			/usr/sbin/dimm_train_fail_log.sh 1
		fi
	elif [ ${boot_status} == "0x01" ]; then
		# Check and set boot progress to dbus
		set_boot_progress $boot_stage $uefi_code
	fi

	# Log POST Code to dbus.
	set_postcode $boot_stage $boot_status $uefi_code

	# Stop the service when booting to OS
	if ([ ${boot_stage} == "0x08" ] && [ ${boot_status} == "0x02" ]);
	then
		update_boot_progress "OSStart"
		log_redfish_biosboot_ok_event
		break
	fi
done

