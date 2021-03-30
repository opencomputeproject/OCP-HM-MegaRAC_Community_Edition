# Expected JSON Checker tool

A tool that cross checks an expected set of JSON entries, by a given index,
with a given input set of JSON entries. In addition, there's an ability to
filter expected entries by using logical operations against entries within
another file contain JSON entries. This filtering functionality on cross
checking expected entries with a set of input is optional

Expected entries that only want to be found within the input JSON should use
a value of `{}`. This denotes the value for the given property should be
treated as a `don't care` and ignored during the cross-check.

## Intention:

The intention for this tool's creation was to provide an ability to cross check
entries within the BMC's enumerated sensor JSON output against an expected set
of entries for a specific machine. In addition, the expected set of entries are
different based on specific entries within inventory. So given a dump of a
machine's enumerated sensor data and inventory data to separate JSON files,
an expected set of entries that should be contained within the sensor data
is created for this machine. These JSON files are then fed into this tool to
determine if all the expected set of entries are found within the sensor data.
The machine's name was used as the index into the expected JSON to allow the
same expected JSON file to be used across multiple machines instead of having
separate expected JSON files per machine(since what's expected will likely be
different across different machines).

## (OPTIONAL) Filtering:

Filters can be used for whether or not a set of entries within the expected
JSON file should be included when cross checking with the input set of JSON
entries. This is useful in allowing a single set of expected entries to be
used and add/remove entries based on some other JSON file's contents.

### Supported logical operations:

- \$and : Performs an AND operation on an array with at least two
          expressions and returns the document that meets all the
          expressions. i.e.) {"$and": [{"age": 5}, {"name": "Joe"}]}
- \$or :  Performs an OR operation on an array with at least two
          expressions and returns the documents that meet at least one of
          the expressions. i.e.) {"$or": [{"age": 4}, {"name": "Joe"}]}
- \$nor : Performs a NOR operation on an array with at least two
          expressions and returns the documents that do not meet any of
          the expressions. i.e.) {"$nor": [{"age": 3}, {"name": "Moe"}]}
- \$not : Performs a NOT operation on the specified expression and
          returns the documents that do not meet the expression.
          i.e.) {"$not": {"age": 4}}

## Example Usage:

Expected JSON set of entries for a `witherspoon` index(expected.json):
```
{
    "witherspoon": {
        "/xyz/openbmc_project/sensors/fan_tach/fan0_0": {
            "Functional": true,
            "Target": {},
            "Value": {}
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan0_1": {
            "Functional": true,
            "Value": {}
        },
        "$op": {
            "$and": [{
                "/xyz/openbmc_project/inventory/system/chassis":{"WaterCooled": false}
            }],
            "$input": [{
                "/xyz/openbmc_project/sensors/fan_tach/fan1_0": {
                    "Functional": true,
                    "Target": {},
                    "Value": {}
                },
                "/xyz/openbmc_project/sensors/fan_tach/fan1_1": {
                    "Functional": true,
                    "Value": {}
                }
            }]
        }
    }
}
```

Input JSON set of entries(input.json):
```
{
    "data": {
        "/xyz/openbmc_project/sensors/fan_tach/fan0_0": {
            "CriticalAlarmHigh": false,
            "CriticalAlarmLow": true,
            "CriticalHigh": 12076,
            "CriticalLow": 2974,
            "Functional": true,
            "MaxValue": 0,
            "MinValue": 0,
            "Scale": 0,
            "Target": 10500,
            "Unit": "xyz.openbmc_project.Sensor.Value.Unit.RPMS",
            "Value": 0
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan0_0/chassis": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis"
            ]
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan0_0/inventory": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0"
            ]
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan0_1": {
            "CriticalAlarmHigh": false,
            "CriticalAlarmLow": false,
            "CriticalHigh": 12076,
            "CriticalLow": 2974,
            "Functional": true,
            "MaxValue": 0,
            "MinValue": 0,
            "Scale": 0,
            "Unit": "xyz.openbmc_project.Sensor.Value.Unit.RPMS",
            "Value": 3393
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan0_1/chassis": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis"
            ]
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan0_1/inventory": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0"
            ]
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan1_0": {
            "CriticalAlarmHigh": false,
            "CriticalAlarmLow": true,
            "CriticalHigh": 12076,
            "CriticalLow": 2974,
            "Functional": true,
            "MaxValue": 0,
            "MinValue": 0,
            "Scale": 0,
            "Target": 10500,
            "Unit": "xyz.openbmc_project.Sensor.Value.Unit.RPMS",
            "Value": 0
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan1_0/chassis": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis"
            ]
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan1_0/inventory": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1"
            ]
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan1_1": {
            "CriticalAlarmHigh": false,
            "CriticalAlarmLow": false,
            "CriticalHigh": 12076,
            "CriticalLow": 2974,
            "Functional": true,
            "MaxValue": 0,
            "MinValue": 0,
            "Scale": 0,
            "Unit": "xyz.openbmc_project.Sensor.Value.Unit.RPMS",
            "Value": 3409
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan1_1/chassis": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis"
            ]
        },
        "/xyz/openbmc_project/sensors/fan_tach/fan1_1/inventory": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1"
            ]
        },
        "/xyz/openbmc_project/sensors/power/ps0_input_power": {
            "CriticalAlarmHigh": false,
            "CriticalAlarmLow": true,
            "CriticalHigh": 2500000000,
            "CriticalLow": 0,
            "Functional": true,
            "MaxValue": 0,
            "MinValue": 0,
            "Scale": -6,
            "Unit": "xyz.openbmc_project.Sensor.Value.Unit.Watts",
            "Value": 0,
            "WarningAlarmHigh": false,
            "WarningAlarmLow": true,
            "WarningHigh": 2350000000,
            "WarningLow": 0
        },
        "/xyz/openbmc_project/sensors/power/ps0_input_power/chassis": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis"
            ]
        },
        "/xyz/openbmc_project/sensors/power/ps0_input_power/inventory": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0"
            ]
        },
        "/xyz/openbmc_project/sensors/power/ps1_input_power": {
            "CriticalAlarmHigh": false,
            "CriticalAlarmLow": false,
            "CriticalHigh": 2500000000,
            "CriticalLow": 0,
            "Functional": true,
            "MaxValue": 0,
            "MinValue": 0,
            "Scale": -6,
            "Unit": "xyz.openbmc_project.Sensor.Value.Unit.Watts",
            "Value": 18000000,
            "WarningAlarmHigh": false,
            "WarningAlarmLow": false,
            "WarningHigh": 2350000000,
            "WarningLow": 0
        },
        "/xyz/openbmc_project/sensors/power/ps1_input_power/chassis": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis"
            ]
        },
        "/xyz/openbmc_project/sensors/power/ps1_input_power/inventory": {
            "endpoints": [
                "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply1"
            ]
        },
        "/xyz/openbmc_project/sensors/temperature/ambient": {
            "CriticalAlarmHigh": false,
            "CriticalAlarmLow": false,
            "CriticalHigh": 35000,
            "CriticalLow": 0,
            "Functional": true,
            "MaxValue": 0,
            "MinValue": 0,
            "Scale": -3,
            "Unit": "xyz.openbmc_project.Sensor.Value.Unit.DegreesC",
            "Value": 22420,
            "WarningAlarmHigh": false,
            "WarningAlarmLow": false,
            "WarningHigh": 25000,
            "WarningLow": 0
        }
    },
    "message": "200 OK",
    "status": "ok"
}
```

Filter JSON set of entries(filter.json):
```
{
    "data": {
        "/xyz/openbmc_project/inventory/system": {
            "AssetTag": "",
            "BuildDate": "",
            "Cached": false,
            "FieldReplaceable": false,
            "Manufacturer": "",
            "Model": "8335-GTA        ",
            "PartNumber": "",
            "Present": true,
            "PrettyName": "",
            "SerialNumber": "1234567         "
        },
        "/xyz/openbmc_project/inventory/system/chassis": {
            "AirCooled": true,
            "Type": "RackMount",
            "WaterCooled": false
        },
        "/xyz/openbmc_project/inventory/system/chassis/activation": {
            "endpoints": [
                "/xyz/openbmc_project/software/224cd310"
            ]
        },
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu0": {
            "BuildDate": "1996-01-01 - 00:00:00",
            "Cached": false,
            "FieldReplaceable": true,
            "Functional": true,
            "Manufacturer": "IBM",
            "Model": "",
            "PartNumber": "02CY211",
            "Present": true,
            "PrettyName": "PROCESSOR MODULE",
            "SerialNumber": "YA1934302447",
            "Version": "22"
        },
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu0/core0": {
            "Associations": [
                [
                    "sensors",
                    "inventory",
                    "/xyz/openbmc_project/sensors/temperature/p0_core0_temp"
                ]
            ],
            "Functional": true,
            "Present": true,
            "PrettyName": ""
        },
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu0/core1": {
            "Associations": [
                [
                    "sensors",
                    "inventory",
                    "/xyz/openbmc_project/sensors/temperature/p0_core1_temp"
                ]
            ],
            "Functional": true,
            "Present": true,
            "PrettyName": ""
        }
    },
    "message": "200 OK",
    "status": "ok"
}
```

Invoke the tool(with everything expected found):
```
> expectedJsonChecker.py witherspoon expected.json input.json -f filter.json
```
Invoke the tool(with modified fan1_0 `Functional` property to `False`):
```
> expectedJsonChecker.py witherspoon expected.json input.json -f filter.json
NOT FOUND:
/xyz/openbmc_project/sensors/fan_tach/fan1_0: {u'Functional': False}
```
