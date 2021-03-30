# Managing LED groups and physical LEDs through BMC

## Overview

LEDs that are present in the system can be managed by:
`BMC / Hardware controller / Operating system`. This document describes how
to control the LEDs that are managed by BMC.

## Understanding LED groups

While it is entirely possible to act directly on a physical LED, it makes
hard when more than one LED is to be acted on for a particular usecase.

For example: Certain systems may have a requirement to **Blink** one particular
LED upon reaching some state. However, some other systems may have a requirement
to **Blink** set of LEDs in different frequencies. Few more systems may have a
requirement to act on set of LEDs with different actions. Meaning, some LEDs
to be solid [ON] while some LEDs to be Blinking etc.

As described above, it is more of a system specific policy on what set
of LEDs to be acted on in different usecases. Taking all that complexity into
the code makes it impossible to extend across implementations and hence the need
of a manager that works on the set of LEDs and policies and that is the Group.

To make concept of Group more evident, here is another example:
Consider a requirement of blinking system Enclosure LED indicating
an internal problem:

If the DIMM has some issues then the system admin needs a way of identifying
that system before acting on it. In one case, the LED group may consist of
DIMM LED + Enclosure Fault LED. However, if the DIMM is attached to a Raiser
card, then the LED group may consist of DIMM LED + Raise card LED + Enclosure
Fault LED. Defining this path will make it easier to locate the box and the
exact part that needs servicing.

Definition of LED groups could be static or generated from MRW and must be in
YAML format. A group definition looks like this:
```
bmc_booted:
    led_1:
      Action: On
      Frequency: 1000
    led_2:
      Action: Blink
      Frequency: 1000
      DutyOn: 50

enclosure_identify:
    id_front:
      Action: Blink
      Frequency: 1000
      DutyOn: 50
    id_rear:
      Action: Blink
      Frequency: 1000
      DutyOn: 50
```
This says that the group `bmc_booted` consists of 2 physical LEDs in it.
When this group is acted on, led_1 will turn solid [ON], while led_2
would be blinking at 50% duty cycle.

## Dbus interfaces for LED groups.

Refer: [specification](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Led/Group.interface.yaml)

There is only one property called **asserted** defined on groups and when set to
boolean **true**, the actions listed for the LEDs in that group will get into
effect as described above.
When the property is set to **false**, the actions are un-done.

* Henceforth, the term **asserted** would mean writing boolean **true**
  onto `assert` property of the group. Term **de-assert** would mean
  writing **false** to `assert` property.

## Group Dbus information

**Path: "/xyz/openbmc_project/led/groups/<name>"**
**Interface: "xyz.openbmc_Project.Led.Group"**

Using the yaml definition above, a user can just set the `asserted` property to
boolean `true` on '/xyz/openbmc_project/led/groups/enclosure_identify' and that
would result in blinking the front and rear Identify LEDs of the enclosure.

## Mandatory groups

It is mandatory that all implementations provide definitions of atleast 2 groups
namely; **bmc_booted** and **power_on**. Those would be asserted post reaching
BMCReady and PowerOn respecively. It is fine to have no LEDs in those groups but
the group as such is deemed required.

Example: The Group yaml may just be;
```
bmc_booted:
power_on:
```

For the IPMI command "chassis identify" to function, **enclosure_identify** must
also be implemented.

## Understanding Physical LEDs

It is always **recommended** that external users use **only** the LED groups.
Describing the Physical LED here just for documenting it and strictly NOT to
be used outside of the firmware code.

## Dbus interfaces for physical LEDs.

Refer: [specification](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Led/Physical.interface.yaml)

## Dbus information

**Path: "/xyz/openbmc_project/led/physical/<name>"**
**Interface: "xyz.openbmc_Project.Led.Physical"**

Using **/xyz/openbmc_project/led/groups/enclosure_identify** as example;
setting the `asserted` property to `true` would result in these actions in
*id_front* and *id_rear* physical LEDs.

1) Property *DutyOn* is set to `50` in;
  `/xyz/openbmc/project/led/physical/id_front` and
  `/xyz/openbmc/project/led/physical/id_rear`

2) Property *State* is set to `xyz.openbmc_project.Led.Physical.Action.On` in
   `/xyz/openbmc/project/led/physical/id_front` and
   `/xyz/openbmc/project/led/physical/id_rear`

Which means, if some test wants to verify if the action on Group really resulted
in acting on physical LED, those are the properties to be read from physical
service.

#                                         END of Document
