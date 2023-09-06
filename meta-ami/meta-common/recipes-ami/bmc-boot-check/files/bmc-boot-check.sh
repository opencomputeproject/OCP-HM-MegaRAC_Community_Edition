#!/bin/sh


# AST2600  SCU64[12] & SCU64[13] will be low when bmc boots up after AC lost.
# Detect if bmc reboot is caused by AC loss, and generate ACLost
# file in '/tmp/' directory, such that during each time bmc boot current 
# status will be available

ADDRESS=0x1e6e2064
ACLOSSFILE=/tmp/ACLost

rstArmBit=$((1<< 12))
rst03Bit=$((1<<13))

# Getting register value
VAL=$(devmem $ADDRESS)
rstArm=$((rstArmBit & VAL))
rst03=$((rst03Bit & VAL))

if (( $rstArm == 0 && $rst03 == 0 ))
then
    `touch $ACLOSSFILE`
fi

