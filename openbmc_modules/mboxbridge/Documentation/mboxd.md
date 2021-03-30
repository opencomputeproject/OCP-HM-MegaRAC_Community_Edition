Copyright 2017,2018 IBM

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## Intro

This document describes the reference mailbox daemon contained in this
repository.

## Files

The main mailbox daemon is implemented in mboxd.c. This file uses helper
functions from the various mboxd_*.c files.

```
dbus.c -    Contains the handlers for the D-Bus commands which the daemon can
                  receive.
flash.c -   Contains the functions for performing flash access including
                  read, write and erase.
lpc.c -     Contains the functions for controlling the LPC bus mapping
                  including pointing the bus so it maps flash and memory.
transport_mbox.c -     Contains the handlers for the mbox commands which the daemon
                  can receive.
windows.c - Contains the functions for managing the window cache.
```

## Daemon State

The daemon is a state machine with 5 valid states:

```
UNINITIALISED -         The daemon is still in the initialisation phase and
                        will not respond
ACTIVE_MAPS_FLASH -     The daemon is polling for incoming commands, is not
                        currently suspended and the LPC bus maps the flash
                        device.
SUSPEND_MAPS_FLASH -    The daemon is polling for incoming commands, is
                        currently suspended and the LPC bus maps the flash
                        device.
ACTIVE_MAPS_MEM -       The daemon is polling for incoming commands, is not
                        currently suspended and the LPC bus maps the reserved
                        memory region.
SUSPEND_MAPS_MEM -      The daemon is polling for incoming commands, is
                        currently suspended and the LPC bus maps the reserved
                        memory region.
```

As described in the protocol, the daemon can be suspended to allow the BMC to
access flash without the daemon interfering. The daemon will still respond to
commands while suspended but won't allow the host to, or itself, access the
flash.

### State Transitions

```
UNINITIALISED -> ACTIVE_MAPS_FLASH -      Uninitiated: Occurs when the daemon is
                                          finished initialising.
ACTIVE_MAPS_FLASH -> SUSPEND_MAPS_FLASH - Suspend command received over
                                          D-Bus
SUSPEND_MAPS_FLASH -> ACTIVE_MAPS_FLASH - Resume command received over
                                          D-Bus
ACTIVE_MAPS_MEM -> SUSPEND_MAPS_MEM -     Suspend command received over D-Bus
SUSPEND_MAPS_MEM -> ACTIVE_MAPS_MEM -     Resume command received over D-Bus
ACTIVE_MAPS_FLASH -> ACTIVE_MAPS_MEM -    GET_MBOX_INFO command received
SUSPEND_MAPS_FLASH -> SUSPEND_MAPS_MEM -  GET_MBOX_INFO command received
ACTIVE_MAPS_MEM -> ACTIVE_MAPS_FLASH -    Reset D-Bus or mbox command received
SUSPEND_MAPS_MEM -> SUSPEND_MAPS_FLASH -  Transition not allowed, we
                                          don't let the host modify flash
                                          while the daemon is suspended.
```

## Window Cache

While the protocol describes that there is only one active window at a time,
the daemon keeps a cache of previously accessed windows to avoid the need to
reload them from flash should the host access them again in the future.

The reserved memory region is divided among a number of windows which make up
the window cache. When the host requests a flash offset the cache is searched
for one which contains that offset. If one is found we simply point the host to
it at the correct LPC offset for that windows location and the requested flash
offset.

If there is no window in the cache which contains the requested flash offset
then we have to find one to load with the requested flash contents. If there
are any windows which are empty then we choose those, otherwise, we choose one to
evict using a least recently used (LRU) scheme. The flash is then copied into
this window and the host pointed at its location on the LPC bus.

When ever the flash is changed we have to invalidate all windows in the window
cache as their contents may no longer accurately reflect those of the flash.

## Daemon Operation

### Invocation

The daemon is invoked on the command line with a few parameters:

```
(*) - required
(#) - optional but recommended
(~) - optional

--flash *       - The size of the PNOR image on the flash device
--window-size # - The size to make each window in the cache
--window-num #  - The number of windows to have in the cache
--verbose ~     - Increase the verbosity with which the daemon runs
--sys-log ~     - Use the system log rather than outputting to the console
```

If any of the required parameters are missing or invalid an error will be
printed and the daemon will terminate.
If window-size and window-num aren't specified then the default is to have a
single window which spans the entire reserved memory region.

### Initialisation

After the command line has been parsed the daemon will initalise its various
interfaces. If any of these initialisations fail it will print an error and the
daemon will terminate.

After initilisation, the daemon points the LPC mapping to the actual flash
device, sets the BMC_READY BMC event and starts polling.

### Polling

The daemon sits in a poll loop waiting for an event to happen on one or more of
the mbox, D-Bus or signal file descriptors.

#### Handling MBOX Commands

When an event occurs on the mbox file descriptor the mbox request is read from
the mbox registers and processed.

The command is validated and then the appropriate handler called. The response
is then written back to the mbox registers to indicate the outcome of the
command and an interrupt is sent to the host.

#### Handling D-Bus Commands

When an event occurs on the D-Bus file descriptor the D-Bus request is read from
the D-Bus interface and processed.

The command is validated and then the appropriate handler called. The response
is then written back to the D-Bus interface to indicate the outcome of the
command.

#### Handling Signals

The daemon blocks SIGINTs, SIGTERMs, and SIGHUPs and instead polls for them on
a signal file descriptor.

When an event occurs on this file descriptor the signal received is determined
and processed as follows:

```
SIGINT -  Terminate the daemon
SIGTERM - Terminate the daemon
SIGHUP -  Clear the window cache and point the LPC bus mapping back to
          flash
```

### Termination

The daemon can be terminated for multiple reasons; invalid command line, unable
to initialise, received SIGINT or SIGTERM, or because it received the kill D-Bus
command.

On termination, the daemon clears the window cache and notifies the host that the
active window has been closed, points the LPC bus mapping back to flash, clears
the BMC_READY BMC event bit and frees all of its allocated resources.
