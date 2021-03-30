# OEM-IPMI Commands to talk to Phosphor-pid-control

## IPMI Command Specification

The host needs the ability to send to the BMC, the margin information on the
devices that it knows how to read that the BMC cannot. There is no command in
IPMI that currently supports this use-case, therefore it will be added as an OEM
command.

The state of the BMC readable temperature sensors can be read through normal
IPMI commands and is already supported.

### OEM Set Control

A host tool needs to be able to set the control of the thermal system to either
automatic or manual. When manual, the daemon will effectively wait to be told to
be put back in automatic mode. It is expected in this manual mode that something
will be controlling the fans via the other commands.

Manual mode is controlled by zone through the following OEM command:

#### Request

Byte | Purpose      | Value
---- | ------------ | -----------------------------------------------------
`00` | `netfn`      | `0x2e`
`01` | `command`    | `0x04 (also using manual command)`
`02` | `oem1`       | `0xcf`
`03` | `oem2`       | `0xc2`
`04` | `padding`    | `0x00`
`05` | `SubCommand` | `Get or Set. Get == 0, Set == 1`
`06` | `ZoneId`     |
`07` | `Mode`       | `If Set, Value 1 == Manual Mode, 0 == Automatic Mode`

#### Response

Byte | Purpose   | Value
---- | --------- | -----------------------------------------------------
`02` | `oem1`    | `0xcf`
`03` | `oem2`    | `0xc2`
`04` | `padding` | `0x00`
`07` | `Mode`    | `If Set, Value 1 == Manual Mode, 0 == Automatic Mode`

### OEM Get Failsafe Mode

A host tool needs to be able to read back whether a zone is in failsafe mode.
This setting is read-only because it's dynamically determined within Swampd per
zone.

Byte | Purpose      | Value
---- | ------------ | ----------------------------------
`00` | `netfn`      | `0x2e`
`01` | `command`    | `0x04 (also using manual command)`
`02` | `oem1`       | `0xcf`
`03` | `oem2`       | `0xc2`
`04` | `padding`    | `0x00`
`05` | `SubCommand` | `Get == 2`
`06` | `ZoneId`     |

#### Response

Byte | Purpose    | Value
---- | ---------- | -----------------------------------------------
`02` | `oem1`     | `0xcf`
`03` | `oem2`     | `0xc2`
`04` | `padding`  | `0x00`
`07` | `failsafe` | `1 == in Failsafe Mode, 0 not in failsafe mode`
