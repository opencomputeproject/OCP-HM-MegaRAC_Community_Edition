#!/bin/bash
syndrome_path() {
    if [ $1 == 0 ]; then
        echo "/sys/bus/platform/drivers/smpro-errmon/1e78a0c0.i2c-bus:smpro@4f:errmon/event_dimm_syndrome"
    else
        echo "/sys/bus/platform/drivers/smpro-errmon/1e78a0c0.i2c-bus:smpro@4e:errmon/event_dimm_syndrome"
    fi
}

function log_ampere_oem_redfish_event()
{
    msg=$1
    priority=$2
    severity=$3
    msgID=$4
    msgArgs1=$5
    msgArgs2=$6
    logger-systemd --journald << EOF
MESSAGE=${msg}
PRIORITY=${priority}
SEVERITY=${severity}
REDFISH_MESSAGE_ID=${msgID}
REDFISH_MESSAGE_ARGS=${msgArgs1},${msgArgs2}
EOF
}

parse_phy_syndrome_s1_type() {
    s1=$1
    slice=$((s1 & 0xf))
    ubit=$(((s1 & 0x10) >> 4))
    lbit=$(((s1 & 0x20) >> 5))
    uMsg="Upper Nibble: No Error"
    lMsg="Lower Nibble: No Error"
    if [ $ubit == 1 ]; then
        uMsg="Upper Nibble: No rising edge error"
    fi
    if [ $lbit == 1 ]; then
        lMsg="Lower Nibble: No rising edge error"
    fi
    echo "Slice $slice: $uMsg, $lMsg"
}

parse_phy_syndrome() {
    s0=$1
    s1=$2
    case $s0 in
    1)
        echo "PHY Training Setup failure"
        ;;
    2)
        s1Msg=$(parse_phy_syndrome_s1_type $s1)
        echo "PHY Write Leveling failure: $s1Msg"
        ;;
    3)
        echo "PHY Read Gate Leveling failure"
        ;;
    4)
        echo "PHY Read Leveling failure"
        ;;
    5)
        echo "PHY Software Training failure"
        ;;
    *)
        echo "N/A"
        ;;
    esac
}

parse_dimm_syndrome() {
    s0=$1
    case $s0 in
    1)
        echo "DRAM VREFDQ Training failure"
        ;;
    2)
        echo "LRDIMM DB Training failure"
        ;;
    3)
        echo "LRDIMM DB Software Training failure"
        ;;
    *)
        echo "N/A"
        ;;
    esac
}

log_err_to_redfish_err() {
    err="$(printf '%d' "0x$1" 2>/dev/null)"
    channel="$(printf '%d' "0x$2" 2>/dev/null)"
    data="$(printf '%d' "0x$3" 2>/dev/null)"
    trErrType=$((data & 0x03))
    rank=$(((data & 0x1C) >> 2))
    syndrome0=$(((data & 0xE0) >> 5))
    syndrome1=$(((data & 0xFF00) >> 8))
    #phy sysdrom errors
    fType=""
    redfisComp="DIMM"
    redfisMsg=""
    if [ $trErrType == 1 ]; then
        fType="PHY training failure"
        redfisMsg=$(parse_phy_syndrome $syndrome0 $syndrome1)
    #dimm traning errors
    elif [ $trErrType == 2 ]; then
        fType="DIMM training failure"
        redfisMsg=$(parse_dimm_syndrome $syndrome0)
    else
        fType="Invalid DIMM Syndrome error type"
        redfisMsg="NA"
    fi
    #smg=$("DDR training: MCU rank $rank: $fType: $redfisMsg")
    log_ampere_oem_redfish_event \
        "" 2 "" "OpenBMC.0.1.AmpereCritical.Critical" \
        $redfisComp "Slot $channel MCU rank $rank: $fType: $redfisMsg"
}

log_err_to_sel_err() {
    channel="$(printf '%d' "0x$2" 2>/dev/null)"
    data="$(printf '%d' "0x$3" 2>/dev/null)"
    byte0=$(((data & 0xff00) >> 8))
    byte1=$((data & 0xff))
    evtdata0=$(($EVENT_DIR_ASSERTION | $OEM_SENSOR_SPECIFIC))
    evtdata1=$((($channel << 4) | $BOOT_SYNDROME_DATA | ($socket << 3)))
    #phy sysdrom errors
    #OEM data bytes
    # oem id: 3 bytes [0x3a 0xcd 0x00]
    # sensor num: 1 bytes
    # sensor type: 1 bytes
    # data bytes: 4 bytes
    # sel type: 4 byte [0x00 0x00 0x00 0xC0]
    busctl call xyz.openbmc_project.Logging.IPMI \
        /xyz/openbmc_project/Logging/IPMI \
        xyz.openbmc_project.Logging.IPMI IpmiSelAddOem \
        sayy "" 12 \
        0x3a 0xcd 0x00 \
        $SENSOR_TYPE_SYSTEM_FW_PROGRESS $SENSOR_BOOT_PROGRESS \
        $evtdata0 $evtdata1 $byte0 $byte1\
        0x00 0x00 0x00 0xC0
}

BOOT_SYNDROME_DATA=4
SENSOR_BOOT_PROGRESS=235
EVENT_DIR_ASSERTION=0x00
EVENT_DIR_DEASSERTION=0x01
OEM_SENSOR_SPECIFIC=0x70
SENSOR_TYPE_SYSTEM_FW_PROGRESS=0x0F

socket=$1
filename=$(syndrome_path $socket)
if [ ! -f $filename ]; then
    echo "Can not find event_dimm_syndrome of socket $socket"
    exit 0;
fi

echo "File syndrome $filename"
n=0
cat $filename | while read line; do
    # reading each line
    n=$((n+1))
    log_err_to_redfish_err ${line[0]} ${line[1]} ${line[2]}
    log_err_to_sel_err ${line[0]} ${line[1]} ${line[2]}
    usleep 300000
done

exit 0;
