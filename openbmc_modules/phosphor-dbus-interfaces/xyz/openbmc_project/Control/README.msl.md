# Minimum Ship Level (MSL)

## Overview

The OpenBMC API provides a global setting for enforcement of minimum ship level.
Typically a setting like this would be used to enable the use of hardware
configurations in platform development that may not necessarily be supported
in a manufacturing or field environment.

If an OpenBMC implementation provides MinimumShipLevelRequired, it must be
on `/xyz/openbmc_project/control/minimum_ship_level_required`.  There are
**no** requirements on how the setting is used; implementations are free to
react to the value of the setting however they choose.

In anticipation of a common use case of examining hardware assets, determining
if they are of a certain revision or part, and taking some action if they are
not, the OpenBMC API also provides an inventory decorator called
MeetsMinimumShipLevel.

MeetsMinimumShipLevel can be implemented on any object in the inventory
namespace.  Again, there are no requirements on what an implementation does
when a particular inventory object does or does not meet the minimum ship
level.

## Implementation Notes

Consider a server platform with an MSL requirement that a fan control card is
a certain revision, and if it is not, the server must not be allowed to
boot, unless a user explicitly specifies otherwise.

Phosphor OpenBMC provides a basic implementation of MinimumShipLevelRequired.
It maintains a setting store and interface for the setting value.  This
implementation is suitable for the hypothetical platform implementation
requirements, allowing a user to toggle the setting back and forth.

To enable verification that MSL has been satisfied, the hypothetical platform
implementation would need to write an application that implements
MeetsMinimumShipLevel on the inventory assets to be verified.  At present
Phosphor OpenBMC does not provide any applications that do this.  If your
application is general purpose and could meet the needs of others, consider
contributing it to Phosphor OpenBMC.  A possible implementation for the
hypothetical fan control card might be an application that validates a number
of hwmon sensors exist and then sets the MinimumShipLevelRequired on the
fan inventory object accordingly.

Finally, the setting and the inventory assets must be compared and the server
prevented from powering on.  This could be accomplished with a custom application
or for simple logic, with
[PDM](https://github.com/openbmc/phosphor-dbus-monitor) rules.
