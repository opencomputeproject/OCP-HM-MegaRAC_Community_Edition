# Entity Manager

Entity manager is a runtime configuration application which parses configuration
files (in JSON format) and attempts to detect the devices described by the
configuration files. It also can, based on the configuration, attempt to load
install devices into sysfs. It takes these configurations and produces a best
representation of the files on dbus using the xyz.openbmc_project.Configuration
namespace. It also produces a system.json file for persistance.

## Configuration Syntax

In most cases a server system is built with multiple hardware modules (circuit
boards) such as baseboard, risers, and hot-swap backplanes. While it is
perfectly legal to combine the JSON configuration information for all the
hardware modules into a single file if desired, it is also possible to divide
them into multilple configuration files. For example, there may be a baseboard
JSON file (describes all devices on the baseboard) and a chassis JSON file
(describes devices attached to the chassis). When one of the hardware modules
needs to be upgraded (e.g., a new temperature sensor), only such JSON
configuration file needs to be be updated.

Within a configuration file, there is a JSON object which consists of multiple
"string : value" pairs. This Entity Manager defines the following strings.

| String    | Example Value                            | Description                              |
| :-------- | ---------------------------------------- | ---------------------------------------- |
| "Name"    | "X1000 1U Chassis"                       | Human readable name used for identification and sorting. |
| "Probe"   | "xyz.openbmc_project.FruDevice({'BOARD_PRODUCT_NAME':'FFPANEL'})" | Statement which attempts to read from d-bus. The result determines if a configuration record should be applied. The value for probe can be set to “TRUE” in the case the record should always be applied, or set to more complex lookups, for instance a field in a FRU file that is exposed by the frudevice |
| "Exposes" | [{"Name" : "CPU fan"}, ...]              | An array of JSON objects which are valid if the probe result is successful. These objects describe the devices BMC can interact. |
| "Status"  | "disabled"                               | An indicator that allows for some records to be disabled by default. |
| "Bind*"  | "2U System Fan connector 1"              | The record isn't complete and needs to be combined with another to be functional. The value is a unique reference to a record elsewhere. |
| "DisableNode| "Fan 1" | Sets the status of another Entity to disabled. |

Template strings in the form of "$identifier" may be used in configuration
files. The following table describes the template strings currently defined.

| Template String | Description                              |
| :-------------- | :--------------------------------------- |
| "$bus"          | During a I2C bus scan and when the "probe" command is successful, this template string is substituted with the bus number to which the device is connected. |
| "$address"   | When the "probe" is successful, this template string is substituted with the (7-bit) I2C address of the FRU device. |
| "$index"        | A run-tim enumeration. This template string is substituted with a unique index value when the "probe" command is successful. This allows multiple identical devices (e.g., HSBPs) to exist in a system but each with a unique name. |



## Configuration HowTos

If you're just getting started and your goal is to add sensors dynamically,
check out [My First Sensors](docs/my_first_sensors.md)


## Configuration Records - Baseboard Example

Required fields are name, probe and exposes.

The configuration JSON files attempt to model after actual hardware modules
which made up a complete system. An example baseboard JSON file shown below
defines two fan connectors and two temperature sensors of TMP75 type. These
objects are considered valid by BMC when the probe command (reads and compares
the product name in FRU) is successful and this baseboard is named as "WFP
baseboard".

```
{
    "Exposes": [
        {
            "Name": "1U System Fan connector 1",
            "Pwm": 1,
            "Status": "disabled",
            "Tachs": [
                1,
                2
            ],
            "Type": "IntelFanConnector"
        },
        {
            "Name": "2U System Fan connector 1",
            "Pwm": 1,
            "Status": "disabled",
            "Tachs": [
                1
            ],
            "Type": "IntelFanConnector"
        },
        {
            "Address": "0x49",
            "Bus": 6,
            "Name": "Left Rear Temp",
            "Thresholds": [
                {
                    "Direction": "greater than",
                    "Name": "upper critical",
                    "Severity": 1,
                    "Value": 115
                },
                {
                    "Direction": "greater than",
                    "Name": "upper non critical",
                    "Severity": 0,
                    "Value": 110
                },
                {
                    "Direction": "less than",
                    "Name": "lower non critical",
                    "Severity": 0,
                    "Value": 5
                },
                {
                    "Direction": "less than",
                    "Name": "lower critical",
                    "Severity": 1,
                    "Value": 0
                }
            ],
            "Type": "TMP75"
        },
        {
            "Address": "0x48",
            "Bus": 6,
            "Name": "Voltage Regulator 1 Temp",
            "Thresholds": [
                {
                    "Direction": "greater than",
                    "Name": "upper critical",
                    "Severity": 1,
                    "Value": 115
                },
                {
                    "Direction": "greater than",
                    "Name": "upper non critical",
                    "Severity": 0,
                    "Value": 110
                },
                {
                    "Direction": "less than",
                    "Name": "lower non critical",
                    "Severity": 0,
                    "Value": 5
                },
                {
                    "Direction": "less than",
                    "Name": "lower critical",
                    "Severity": 1,
                    "Value": 0
                }
            ],
            "Type": "TMP75"
        }
    ],
    "Name": "WFP Baseboard",
    "Probe": "xyz.openbmc_project.FruDevice({'BOARD_PRODUCT_NAME' : '.*WFT'})"
}
```

[Full Configuration](https://github.com/openbmc/entity-manager/blob/master/configurations/WFT%20Baseboard.json)


#### Configuration Records - Chassis Example

Although fan connectors are considered a part of a baseboard, the physical fans
themselves are considered as a part of a chassis. In order for a fan to be
matched with a fan connector, the keyword "Bind" is used. The example below
shows how a chassis fan named "Fan 1" is connected to the connector named "1U
System Fan connector 1". When the probe command finds the correct product name
in baseboard FRU, the fan and the connector are considered as being joined
together.

```
{
    "Exposes": [
        {
            "BindConnector": "1U System Fan connector 1",
            "Name": "Fan 1",
            "Thresholds": [
                {
                    "Direction": "less than",
                    "Name": "lower critical",
                    "Severity": 1,
                    "Value": 1750
                },
                {
                    "Direction": "less than",
                    "Name": "lower non critical",
                    "Severity": 0,
                    "Value": 2000
                }
            ],
            "Type": "AspeedFan"
        }
    ]
}
```

## Enabling Sensors

As daemons can trigger off of shared types, sometimes some handshaking will be
needed to enable sensors. Using the TMP75 sensor as an example, when the sensor
object is enabled, the device tree must be updated before scanning may begin.
The entity-manager can key off of different types and export devices for
specific configurations. Once this is done, the baseboard temperature sensor
daemon can scan the sensors.

