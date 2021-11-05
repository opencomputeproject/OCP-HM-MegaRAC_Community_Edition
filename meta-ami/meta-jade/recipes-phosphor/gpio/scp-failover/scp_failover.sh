#!/bin/bash

# Description: This script will handle FailOver Event
#             , use the logger-systemd to log Redfish event

# Author: Chanh Nguyen <chnguyen@amperecomputing.com>

source /usr/sbin/gpio-lib.sh

LOGGER_PATH="/usr/bin/logger-systemd"
evt="OpenBMC.0.1.AmpereCritical.Critical"
sev="Critical"
redfish_mess_arg="SCP Authentication failure detected"

I2C_BACKUP_SEL=$(gpio_get_val 8)

if [[ $? -ne 0 ]]; then
  echo "ERROR: GPIO I2C_BACKUP_SEL used"
  exit 1
fi

# Check the I2C_BACKUP_SEL
if [ ${I2C_BACKUP_SEL} == "1" ]; then
  # If it is HIGH, set it LOW. Then reset the Host to boot from
  # the failover Boot EEPROM.
  echo "Failover event: switch HOST boot from Failover-EEPROM"
  gpio_configure_output 8 0

  busctl set-property \
  xyz.openbmc_project.State.Host /xyz/openbmc_project/state/host0 \
  xyz.openbmc_project.State.Host RequestedHostTransition \
  s xyz.openbmc_project.State.Host.Transition.ForceWarmReboot
else
  # If it is LOW, log an Redfish event using logger-systemd
  logger-systemd --journald << EOF
MESSAGE=
PRIORITY=2
SEVERITY=${sev}
REDFISH_MESSAGE_ID=${evt}
REDFISH_MESSAGE_ARGS=${redfish_mess_arg}
EOF
fi
