#!/bin/bash -x

set -euo pipefail

CONNECT="$@"

i=0

while true;
do
    echo Boot $i

    ssh ${CONNECT} /usr/sbin/obmcutil --wait poweron
    time expect petitboot.exp -- ${CONNECT}
    ssh ${CONNECT} /usr/sbin/obmcutil --wait poweroff

    i=$(($i + 1))

    echo
done

