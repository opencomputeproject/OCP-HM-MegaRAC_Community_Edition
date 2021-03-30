# BMC, Host, and Chassis State Management

## Overview

The goal of the phosphor-state-manager repository is to control and track the
states of the different software entities in a system.  All users will usually
implement the BMC state interfaces, and some, when creating servers will do the
Host and Chassis state interfaces.  These interfaces will be the mechanism by
which you determine the state of their corresponding instances, as well as
reboot the BMC and hosts, and turn on and off power to the chassis.  The
interfaces are designed in a way to support a many to many mappings of each
interface.

There are three states to track and control on a BMC based server.  The states
below in () represent the actual parameter name as found in
`/xyz/openbmc_project/state/`+`/bmcX,/hostY,/chassisZ` where X,Y,Z are the
instances (in most cases 0).  For all three states, the software tracks a
current state, and a requested transition.

1. *BMC* : The BMC has either started all required systemd services and reached
it's required target (Ready) or it's on it's way there (NotReady).  Users can
request a (Reboot).

2. *Host* : The host is either (Off), (Running), or it's (Quiesced).
Running simply implies that the processors are executing instructions.  Users
can request the host be in a (Off), (On), or (Reboot) state.  More details on
different Reboot options below.
Quiesced means the host OS is in a quiesce state and the system should be
checked for errors. For more information refer to
[Error Handling of systemd](https://github.com/openbmc/docs/blob/master/architecture/openbmc-systemd.md#error-handling-of-systemd)

3. *Chassis* : The chassis is either (Off) or (On)
This represents the state of power to the chassis.  The Chassis being on
is a pre-req to the Host being running.  Users can request for the chassis to be
(Off) or (On).  A transition to one or the other is implied by the transition
not matching the current state.

A simple system design would be to include a single *BMC*, *Host*, and
*Chassis*.

Details of the properties and their valid settings can be found in the state
manager dbus interface [specification](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/State/)

### BMC

The *BMC* would provide interfaces at
`/xyz/openbmc_project/state/bmc<instance>`

### *Host*

The *Host* would provide interfaces at
`/xyz/openbmc_project/state/host<instance>`

### *Chassis*

The *Chassis* would provide interfaces at
`/xyz/openbmc_project/state/chassis<instance>`

### *Chassis System*
This is an instance under *Chassis* and provide interface at
`/xyz/openbmc_project/state/chassis_system<instance>`

Instance 0 (chassis_system0) will be treated as a complete chassis system
which will include BMC, host and chassis. This will support hard power
cycle of complete system.

In multi-host or multi-chassis system, instance number can be used from
1-N, as 0 is reserved for complete system. In multi chassis system this
can be named as chassis_system1 to chassis_systemN

## BMC to Host to Chassis Mapping

In the future, OpenBMC will provide an association API, which allows one
to programmatically work out the mapping between BMCs, Chassis and Hosts.

In order to not introduce subtle bugs with existing API users, `bmc0`,
`chassis0` and `host0` are special. If they exist, they are guaranteed to talk
to the system as a whole as if it was a system with one BMC, one chassis and
one host. If there are multiple hosts, then bmc0/chassis0/host0
will *not* exist. In the event of multiple BMCs or Chassis, bmc0 and chassis0
will act on all entities as if they are one (if at all possible).

This behaviour means that existing code will continue to work, or error out
if the request would be ambiguous and probably not what the user wanted.

For example, if a system has two chassis, only powering off the first chassis
(while leaving the second chassis on) is certainly *not* what the API user had
in mind as they likely desired to hard power off the system. In such a
multi-chassis system, starting counting from 1 rather than 0 would avoid this
problem, while allowing an API user to *intentionally* only power off one
chassis. With chassis0 being special, it would allow existing code to continue
to function on this multi-chassis system.

For example, a system with multiple hosts would have BMCs, Chassis and Hosts
*all* start numbering from 1 rather than 0. This is because multiple hosts
could be in the same chassis, or controlled by the same BMC, so taking action
against them would *not* be what the API user intended.

It is safe to continue to write code referencing bmc0, host0 and
chassis0 and that code will continue to function, or error out rather than
doing something undesirable.

## Hard vs. Soft Power Off

A hard power off is where you simply cut power to a chassis.  You don't give
the software running on that chassis any chance to cleanly shut down.
A soft power off is where you send a notification to the host that is running
on that chassis that a shutdown is requested, and wait for that host firmware
to indicate it has shut itself down.  Once complete, power is then removed
from the chassis. By default, a host off or reboot request does the soft
power off.  If a user desires a cold reboot then they should simply issue a
power off transition request to the chassis, and then issue an on transition
request to the host.

## Operating System State and Boot Progress Properties

[OperatingSystemState](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/State/OperatingSystem/Status.interface.yaml)
property is used to track different progress states of the OS or the hypervisor
boot, while the [BootProgress](https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/xyz/openbmc_project/State/Boot/Progress.interface.yaml)
property is used mainly for indicating system firmware boot progress.
The enumerations used in both the cases are influenced by standard interfaces
like IPMI 2.0 (Section 42.2, Sensor Type Codes and Data, Table 42, Base OS
boot status) and PLDM state spec (DSP0249, Section 6.3, State Sets Tables,
Table 7 – Boot-Related State Sets, Set ID - 196 Boot Progress).

