# How to Configure Phosphor-pid-control

A system needs two groups of configurations: zones and sensors.

## Json Configuration

The json object should be a dictionary with two keys, `sensors` and `zones`.
`sensors` is a list of the sensor dictionaries, whereas `zones` is a list of
zones.

### Sensors

```
"sensors" : [
    {
        "name": "fan1",
        "type": "fan",
        "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1",
        "writePath": "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm1",
        "min": 0,
        "max": 255,
        "ignoreDbusMinMax": true
    },
    {
        "name": "fan2",
        "type": "fan",
        "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan2",
        "writePath": "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm2",
        "min": 0,
        "max": 255,
        "timeout": 4,
    },
...
```

A sensor has a `name`, a `type`, a `readPath`, a `writePath`, a `minimum` value,
a `maximum` value, a `timeout`, and a `ignoreDbusMinMax` value.

The `name` is used to reference the sensor in the zone portion of the
configuration.

The `type` is the type of sensor it is. This influences how its value is
treated. Supports values are: `fan`, `temp`, and `margin`.

**TODO: Add further details on what the types mean.**

The `readPath` is the path that tells the daemon how to read the value from this
sensor. It is optional, allowing for write-only sensors. If the value is absent
or `None` it'll be treated as a write-only sensor.

If the `readPath` value contains: `/xyz/openbmc_project/extsensors/` it'll be
treated as a sensor hosted by the daemon itself whose value is provided
externally. The daemon will own the sensor and publish it to dbus. This is
currently only supported for `temp` and `margin` sensor types.

If the `readPath` value contains: `/xyz/openbmc_project/` (this is checked after
external), then it's treated as a passive dbus sensor. A passive dbus sensor is
one that listens for property updates to receive its value instead of actively
reading the `Value` property.

If the `readPath` value contains: `/sys/` this is treated as a directly read
sysfs path. There are two supported paths:

*   `/sys/class/hwmon/hwmon0/pwm1`
*   `/sys/devices/platform/ahb/1e786000.pwm-tacho-controller/hwmon/<asterisk
    asterisk>/pwm1`

The `writePath` is the path to set the value for the sensor. This is only valid
for a sensor of type `fan`. The path is optional. If can be empty or `None`. It
then only supports two options.

If the `writePath` value contains: `/sys/` this is treated as a directory
written sysfs path. There are two support paths:

*   `/sys/class/hwmon/hwmon0/pwm1`
*   `/sys/devices/platform/ahb/1e786000.pwm-tacho-controller/hwmon/<asterisk
    asterisk>/pwm1`

If the `writePath` value contains: `/xyz/openbmc_project/sensors/fan_tach/fan{N}` it
sets of a sensor object that writes over dbus to the
`xyz.openbmc_project.Control.FanPwm` interface. The `writePath` should be the
full object path.

```
busctl introspect xyz.openbmc_project.Hwmon-1644477290.Hwmon1 /xyz/openbmc_project/sensors/fan_tach/fan1 --no-pager
NAME                                TYPE      SIGNATURE RESULT/VALUE                             FLAGS
org.freedesktop.DBus.Introspectable interface -         -                                        -
.Introspect                         method    -         s                                        -
org.freedesktop.DBus.Peer           interface -         -                                        -
.GetMachineId                       method    -         s                                        -
.Ping                               method    -         -                                        -
org.freedesktop.DBus.Properties     interface -         -                                        -
.Get                                method    ss        v                                        -
.GetAll                             method    s         a{sv}                                    -
.Set                                method    ssv       -                                        -
.PropertiesChanged                  signal    sa{sv}as  -                                        -
xyz.openbmc_project.Control.FanPwm  interface -         -                                        -
.Target                             property  t         255                                      emits-change writable
xyz.openbmc_project.Sensor.Value    interface -         -                                        -
.MaxValue                           property  x         0                                        emits-change writable
.MinValue                           property  x         0                                        emits-change writable
.Scale                              property  x         0                                        emits-change writable
.Unit                               property  s         "xyz.openbmc_project.Sensor.Value.Uni... emits-change writable
.Value                              property  x         2823                                     emits-change writable
```

The `minimum` and `maximum` values are optional. When `maximum` is non-zero it
expects to write a percentage value converted to a value between the minimum and
maximum.

The `timeout` value is optional and controls the sensor failure behavior. If a
sensor is a fan the default value is 2 seconds, otherwise it's 0. When a
sensor's timeout is 0 it isn't checked against a read timeout failure case. If a
sensor fails to be read within the timeout period, the zone goes into failsafe
to handle the case where it doesn't know what to do -- as it doesn't have all
its inputs.

The `ignoreDbusMinMax` value is optional and defaults to false.  The dbus
passive sensors check for a `MinValue` and `MaxValue` and scale the incoming
values via these.  Setting this property to true will ignore `MinValue` and
`MaxValue` from dbus and therefore won't call any passive value scaling.

### Zones

```
"zones" : [
        {
            "id": 1,
            "minThermalOutput": 3000.0,
            "failsafePercent": 75.0,
            "pids": [],
...
```

Each zone has its own fields, and a list of PIDs.

| field              | type      | meaning                                   |
| ------------------ | --------- | ----------------------------------------- |
| `id`               | `int64_t` | This is a unique identifier for the zone. |
| `minThermalOutput` | `double`  | This is the minimum value that should be considered from the thermal outputs.  Commonly used as the minimum fan RPM.|
| `failsafePercent`  | `double`  | If there is a fan PID, it will use this value if the zone goes into fail-safe as the output value written to the fan's sensors.|

The `id` field here is used in the d-bus path to talk to the
`xyz.openbmc_project.Control.Mode` interface.

***TODO:*** Examine how the fan controller always treating its output as a
percentage works for future cases.

### PIDs

There are a few PID types: `fan`, `temp`, `margin`, and `stepwise`.

The `fan` PID is meant to drive fans. It's expecting to get the maximum RPM
setpoint value from the owning zone and then drive the fans to that value.

A `temp` PID is meant to drive the RPM setpoint given an absolute temperature
value (higher value indicates a warmer temperature).

A `margin` PID is meant to drive the RPM setpoint given a margin value (lower
value indicates a warmer temperature).

The setpoint output from the thermal controllers is called `RPMSetpoint()`
However, it doesn't need to be an RPM value.

***TODO:*** Rename this method and others to not say necessarily RPM.

Some PID configurations have fields in common, but may be interpreted
differently.

#### PID Field

If the PID `type` is not `stepwise` then the PID field is defined as follows:

| field                | type     | meaning                                   |
| -------------------- | -------- | ----------------------------------------- |
| `samplePeriod`       | `double` | How frequently the value is sampled. 0.1 for fans, 1.0 for temperatures.|
| `proportionalCoeff`  | `double` | The proportional coefficient.             |
| `integralCoeff`      | `double` | The integral coefficient.                 |
| `feedFwdOffsetCoeff` | `double` | The feed forward offset coefficient.      |
| `feedFwdGainCoeff`   | `double` | The feed forward gain coefficient.        |
| `integralLimit_min`  | `double` | The integral minimum clamp value.         |
| `integralLimit_max`  | `double` | The integral maximum clamp value.         |
| `outLim_min`         | `double` | The output minimum clamp value.           |
| `outLim_max`         | `double` | The output maximum clamp value.           |
| `slewNeg`            | `double` | Negative slew value to dampen output.     |
| `slewPos`            | `double` | Positive slew value to accelerate output. |

The units for the coefficients depend on the configuration of the PIDs.

If the PID is a `margin` controller and its `setpoint` is in centigrade and
output in RPM: proportionalCoeff is your p value in units: RPM/C and integral
coefficient: RPM/C sec

If the PID is a fan controller whose output is pwm: proportionalCoeff is %/RPM
and integralCoeff is %/RPM sec.

***NOTE:*** The sample periods are specified in the configuration as they are
used in the PID computations, however, they are not truly configurable as they
are used for the update periods for the fan and thermal sensors.

#### type == "fan"

```
"name": "fan1-5",
"type": "fan",
"inputs": ["fan1", "fan5"],
"setpoint": 90.0,
"pid": {
...
}
```

The type `fan` builds a `FanController` PID.

| field      | type              | meaning                                     |
| ---------- | ----------------- | ------------------------------------------- |
| `name`     | `string`          | The name of the PID. This is just for humans and logging.|
| `type`     | `string`          | `fan`                                       |
| `inputs`   | `list of strings` | The names of the sensor(s) that are used as input and output for the PID loop.|
| `setpoint` | `double`          | Presently UNUSED                            |
| `pid`      | `dictionary`      | A PID dictionary detailed above.            |

#### type == "temp"

***TODO:*** Add notes for temperature configuration.

#### type == "margin"

```
"name": "fleetingpid0",
"type": "margin",
"inputs": ["fleeting0"],
"setpoint": 10,
"pid": {
...
}
```

The type `margin` builds a `ThermalController` PID.

| field      | type              | meaning                                     |
| ---------- | ----------------- | ------------------------------------------- |
| `name`     | `string`          | The name of the PID. This is just for humans and logging.|
| `type`     | `string`          | `margin`                                    |
| `inputs`   | `list of strings` | The names of the sensor(s) that are used as input for the PID loop.|
| `setpoint` | `double`          | The setpoint value for the thermal PID. The setpoint for the margin sensors.|
| `pid`      | `dictionary`      | A PID dictionary detailed above.            |

The output of a `margin` PID loop is that it sets the setpoint value for the
zone. It does this by adding the value to a list of values. The value chosen by
the fan PIDs (in this cascade configuration) is the maximum value.

#### type == "stepwise"

***TODO:*** Write up `stepwise` details.
