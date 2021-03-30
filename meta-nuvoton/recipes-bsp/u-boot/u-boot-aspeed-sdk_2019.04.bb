DESCRIPTION = "u-boot for Aspeed"
SECTION = "kernel"
LICENSE = "GPLv2"

# remove depenency on aspeed .bbappend receipts
EXCLUDE_FROM_WORLD = "1"
