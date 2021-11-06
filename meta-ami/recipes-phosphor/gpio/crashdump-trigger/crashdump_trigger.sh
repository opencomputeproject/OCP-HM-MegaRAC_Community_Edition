#!/bin/sh
#Trigger the crashdump fetch

SERVICE="com.intel.crashdump"
INTERFACE="com.intel.crashdump.OnDemand"
OBJECT="/com/intel/crashdump"
METHOD="GenerateOnDemandLog"

# call the Method
busctl call $SERVICE $OBJECT $INTERFACE $METHOD
