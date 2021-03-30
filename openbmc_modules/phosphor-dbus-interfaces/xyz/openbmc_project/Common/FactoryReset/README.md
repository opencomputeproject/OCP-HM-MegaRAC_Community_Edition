# Factory Reset

## Overview

The OpenBMC API defines a factory reset interface, which is intended to be used
to restore the BMC to its original manufacturer settings. This interface is
defined generically; it is specifically and variously implemented throughout
OpenBMC services, which allows these services to be individually restored to
factory defaults as needed.

## Known Implementations (listed by D-Bus service)

### xyz.openbmc_project.Network
Path: `/xyz/openbmc_project/network`
The network factory reset overwrites the configuration for all configured
network interfaces to a DHCP setting. Configuration changes will take effect
the next time each interface is brought up - either manually or during a BMC
reboot.

### xyz.openbmc_project.Software.BMC.Updater
Path: `/xyz/openbmc_project/software`
The BMC software updater factory reset clears any volumes and persistence files 
created by the BMC processes. This reset occurs only on the next BMC reboot.
