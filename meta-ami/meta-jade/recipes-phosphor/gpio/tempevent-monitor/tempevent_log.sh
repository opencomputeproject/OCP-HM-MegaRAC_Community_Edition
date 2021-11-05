#!/bin/bash

# Description: This script will send log to Event Redfish
#.             , use the logger-systemd

# Syntax: tempevent_log.sh [event] [action]
# Where: [event] is "Hightemp" or "Overtemp"
#.       [action] is "Start" or "Stop"

# Author: Chanh Nguyen <chnguyen@amperecomputing.com>

LOGGER_PATH="/usr/bin/logger-systemd"
evt="OpenBMC.0.1.AmpereCritical.Critical"
sev="Critical"
redfish_mess_arg_1=""
redfish_mess_arg_2=""

if [ $# -eq 0 ]; then
  echo 'No argument is given' >&2
  exit 1
fi

redfish_mess_arg_1="${1} event"

if [ "$2" != "" ]; then
    redfish_mess_arg_2=$2
fi

# Check if the LOGGER_PATH
if [ -e ${LOGGER_PATH} ]; then

  logger-systemd --journald << EOF
MESSAGE=
PRIORITY=2
SEVERITY=${sev}
REDFISH_MESSAGE_ID=${evt}
REDFISH_MESSAGE_ARGS=${redfish_mess_arg_1},${redfish_mess_arg_2}
EOF

else
  echo "Error: Not found ${LOGGER_PATH}"
  exit 1
fi
