# Chassis Power Control

## Overview
Chassis Power Control service exposes D-Bus methods for chassis power operations

### Power Button Interface
Power button interface `xyz.openbmc_project.Chassis.Buttons.Power`
provides following methods, signals.

#### methods
* simPress - To emulate physical power button press.
* simLongPress - To emulate physical power button long press.

#### signals
* Released - Power button released signal.
* Pressed - Power button pressed signal.
* PressedLong - Power button long pressed signal.

### ID Button Interface
ID button interface `xyz.openbmc_project.Chassis.Buttons.ID`
provides following methods, signals.

#### methods
* simPress - To emulate ID button press.

#### signals
* Released - ID button released signal.
* Pressed - ID button pressed signal.

### Reset Button Interface
ID button interface `xyz.openbmc_project.Chassis.Buttons.Reset`
provides following methods, signals.

#### methods
* simPress - To emulate reset button press.

#### signals
* Released - Reset button released signal.
* Pressed - Reset button pressed signal.