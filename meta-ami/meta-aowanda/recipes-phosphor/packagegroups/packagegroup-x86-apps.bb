SUMMARY = "OpenBMC for X86 - Applications"
PR = "r1"

inherit packagegroup

PROVIDES = "${PACKAGES}"
PACKAGES = " \
        ${PN}-chassis \
        ${PN}-fans \
        ${PN}-flash \
        ${PN}-system \
        "

PROVIDES += "virtual/obmc-chassis-mgmt"
PROVIDES += "virtual/obmc-fan-mgmt"
PROVIDES += "virtual/obmc-flash-mgmt"
PROVIDES += "virtual/obmc-system-mgmt"

RPROVIDES_${PN}-chassis += "virtual-obmc-chassis-mgmt"
RPROVIDES_${PN}-fans += "virtual-obmc-fan-mgmt"
RPROVIDES_${PN}-flash += "virtual-obmc-flash-mgmt"
RPROVIDES_${PN}-system += "virtual-obmc-system-mgmt"

SUMMARY_${PN}-chassis = "X86 Chassis"
RDEPENDS_${PN}-chassis = " \
        x86-power-control \
        "
#        obmc-host-failure-reboots \
#
SUMMARY_${PN}-fans = "OpenPOWER Fans"
RDEPENDS_${PN}-fans = " \
        phosphor-pid-control \
        "

SUMMARY_${PN}-flash = "OpenPOWER Flash"
RDEPENDS_${PN}-flash = " \
        phosphor-ipmi-flash \
        "

SUMMARY_${PN}-system = "X86 System"
RDEPENDS_${PN}-system = " \
        bmcweb \
        phosphor-webui \
        "
#        pldm \
#        mctp \
#