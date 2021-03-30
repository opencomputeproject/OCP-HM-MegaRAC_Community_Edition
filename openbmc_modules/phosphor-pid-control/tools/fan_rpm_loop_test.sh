#!/bin/bash

# shim system-level operations to allow for unit testing
MkDir()     { mkdir     "$@"; }
Mv()        { mv        "$@"; }
Sleep()     { sleep     "$@"; }
SystemCtl() { systemctl "$@"; }

# Store commanded rpm as thermal set-point.
#
# Arg:
#   rpm - rpm set-point.
CommandRpm() {
  printf '%d\n' "$1" > /etc/thermal.d/set-point
}

# Sweep or step rpm over range.
#
# Args:
#   start_rpm: start RPM value
#   stop_rpm: stop RPM value
#   num_steps: number of control steps; i.e., not including start.
#   dwell: dwell time at each point, in seconds
RunRpmSteps() {
  local -i start_rpm=$1
  local -i stop_rpm=$2
  local -i num_steps=$3
  local -i dwell=$4

  # Rounding offset when dividing range into num_steps
  local -i h=$((num_steps / 2))

  # To avoid asymmetrical division behavior for negative numbers,
  # rearrange negative slope to positive slope running backward;
  # I.e., to run using loop variable p: {num_steps downto 0}.
  local -i rpm0=$((start_rpm))
  local -i range=$((stop_rpm - start_rpm))
  local -i s=1
  if ((range < 0)); then
    ((rpm0 = stop_rpm))
    ((range = -range))
    ((s = -1))
  fi

  echo "Running RPM from ${start_rpm} to ${stop_rpm} in ${num_steps} steps"
  CommandRpm "${start_rpm}"
  SystemCtl start phosphor-pid-control.service
  Sleep 60

  local i
  for ((i = 0; i <= num_steps; ++i)); do
    local -i p=$((s < 0 ? num_steps - i : i))
    local -i rpm=$((rpm0 + (range * p + h) / num_steps))
    echo "Setting RPM to ${rpm} and sleep ${dwell} seconds"
    CommandRpm "${rpm}"
    Sleep "${dwell}"
  done

  SystemCtl stop phosphor-pid-control.service
  Mv /tmp/swampd.log ~/"${start_rpm}_${stop_rpm}_${num_steps}_${dwell}.csv"
  echo "Done!!"
}

# Sweep and step fans from min to max and max to min.
#
# Args:
#   min_rpm: min RPM for the platform
#   max_rpm: max RPM for the platform
main() {
  local min_rpm=$1
  local max_rpm=$2

  if ((min_rpm < 1 || max_rpm < min_rpm)); then
    echo "Invalid arguments. Usage: fan_rpm_loop_test.sh <MIN_RPM> <MAX_RPM>"
    return 1
  fi

  MkDir -p /etc/thermal.d/
  SystemCtl stop phosphor-pid-control.service

  RunRpmSteps "${min_rpm}" "${max_rpm}" 10 30
  RunRpmSteps "${max_rpm}" "${min_rpm}" 10 30
  RunRpmSteps "${min_rpm}" "${max_rpm}"  1 30
  RunRpmSteps "${max_rpm}" "${min_rpm}"  1 30
}

return 0 2>/dev/null
main "$@"
