# Phosphor State Manager Documentation

This repository contains the software responsible for tracking and controlling
the state of different objects within OpenBMC. This currently includes the
BMC, Chassis, and Host. The most critical feature of phosphor-state-manager
software is its support for requests to power on and off the system by the user.

This software also enforces any restore policy (i.e. auto power on system after
a system power event or bmc reset) and ensures its states are updated correctly
in situations where the BMC is rebooted and the chassis or host are in
on/running states.

This repository also provides a command line tool, obmcutil, which provides
basic command line support to query and control phosphor-state-manager
applications running within an OpenBMC system. This tool itself runs within an
OpenBMC system and utilizes D-Bus APIs. These D-Bus APIs are used for
development and debug and are not intended for end users.

As with all OpenBMC applications, interfaces and properties within
phosphor-state-manager are D-Bus interfaces. These interfaces are then used
by external interface protocols, such as Redfish and IPMI, to report and
control state to/by the end user.

## State Tracking and Control

phosphor-state-manager makes extensive use of systemd. There is a writeup
[here][1] with an overview of systemd and its use by OpenBMC.

phosphor-state-manager follows some basics design guidelines in its
implementation and use of systemd:
- Keep the different objects as independent as possible (host, chassis, bmc)
- Use systemd targets for everything and keep the code within
  phosphor-state-manager minimal
- Ensure it can support required external interfaces, but don't necessarily
  create 1x1 mappings otherwise every external interface will end up with its
  own special chassis or host state request
- If something like a hard power off can be done by just turning off the
  chassis, don't provide a command in the host to do the same thing

phosphor-state-manager implements states and state requests as defined in
phosphor-dbus-interfaces for each object it supports.
- [bmc][2]: The BMC has very minimal states. It is `Ready` once all services
  within the default.target have executed. The only state change request you
  can make of the BMC is for it to reboot itself.
  - CurrentBMCState: NotReady, Ready
  - RequestedBMCTransition: Reboot
- [chassis][3]: The chassis represents the physical hardware in which the system
  is contained. It usually has the power supplies, fans, and other hardware
  associated with it. It can be either `On` or `Off`.
  - CurrentPowerState: On, Off
  - RequestedPowerTransition: On, Off
- [host][4]: The host represents the software running on the system. In most
  cases this is an operating system of some sort. The host can be `Off`,
  `Running`, `Quiesced`(error condition), or in `DiagnosticMode`(collecting
  diagnostic data for a failure)
  - CurrentHostState: Off, Running, Quiesced, DiagnosticMode
  - RequestedHostTransition: Off, On, Reboot, GracefulWarmReboot,
    ForceWarmReboot

As noted above, phosphor-state-manager provides a command line tool,
[obmcutil][5], which takes a `state` parameter. This will use D-Bus commands to
retrieve the above states and present them to the user. It also provides other
commands which will send the appropriate D-Bus commands to the above properties
to power on/off the chassis and host (see `obmcutil --help` within an OpenBMC
system).

The above objects also implement other D-Bus objects like power on hours, boot
progress, reboot attempts, and operating system status. These D-Bus objects are
also defined out in the phosphor-dbus-interfaces repository.

## Restore Policy on Power Events

The [RestorePolicy][6] defines the behavior the user wants when the BMC is
reset. If the chassis or host is on/running then this service will not run.
If they are off then the `RestorePolicy` will be read and executed by
phosphor-state-manager code.

## BMC Reset with Host and/or Chassis On

In situations where the BMC is reset and the chassis and host are on and
running, its critical that the BMC software do two things:
- Never impact the state of the system (causing a power off of a running system
is very bad)
- Ensure the BMC, Chassis, and Host states accurately represent the state of the
system.

Note that some of this logic is provided via service files in system-specific
meta layers. That is because the logic to determine if the chassis is on or
if the host is running can vary from system to system. The requirement to
create the files defined below and ensure the common targets go active is a
must for anyone wishing to enable this feature.

phosphor-state-manager discovers state vs. trying to cache and save states. This
ensure it's always getting the most accurate state information. It discovers the
chassis state by checking the `pgood` value from the power application. If it
determines that power is on then it will do the following:
- Create a file called /run/openbmc/chassis@0-on
  - The presence of this file tells the services to alter their behavior because
    the chassis is already powered on
- Start the obmc-chassis-poweron@0.target
  - The majority of services in this target will "fake start" due to the file
    being present. They will report to systemd that they started and ran
    successfully but they actually do nothing. This is what you would want in
    this case. Power is already on so you don't want to run the services to turn
    power on. You do want to get the obmc-chassis-poweron@0.target in the Active
    state though so that the chassis object within phosphor-state-manager will
    correctly report that the chassis is `On`
- Start a service to check if the host is on

The chassis@0-on file is removed when power is removed from the chassis.

The logic to check if the host is on sends a command to the host, and if a
response is received then similar logic to chassis is done:
- Create a file called /run/openbmc/host@0-on
- Start the obmc-host-start@0.target
  - Similar to above, most services will not run due to the file being created
    and their service files implementing a
    "ConditionPathExists=!/run/openbmc/host@0-request"

The host@0-on file is removed when the host is stopped.

## Building the Code
```
To build this package, do the following steps:

    1. meson build
    2. ninja -C build

To clean the repository again run `rm -rf build`.
```

[1]: https://github.com/openbmc/docs/blob/master/architecture/openbmc-systemd.md
[2]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/BMC.interface.yaml
[3]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/Chassis.interface.yaml
[4]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/State/Host.interface.yaml
[5]: https://github.com/openbmc/phosphor-state-manager/blob/master/obmcutil
[6]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Control/Power/RestorePolicy.interface.yaml
