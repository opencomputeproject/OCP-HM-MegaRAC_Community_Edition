## dbus-pcap: A tool to analyse D-Bus traffic captures

`dbus-pcap` is a tool to slice, dice and display captures of D-Bus traffic
captured into a the standard `pcap` packet container.

D-Bus traffic on OpenBMC can be captured using `busctl`:

```
# busctl capture > /tmp/dbus.pcap
```

## Use

```
$ ./dbus-pcap --help
usage: dbus-pcap [-h] [--json] [--no-track-calls] file [expressions [expressions ...]]

positional arguments:
  file              The pcap file
  expressions       DBus message match expressions

optional arguments:
  -h, --help        show this help message and exit
  --json            Emit a JSON representation of the messages
  --no-track-calls  Make a call response pass filters
```

### Examples of Simple Invocations and Output

The default output style:
```
$ ./dbus-pcap dbus.pcap | head -n 3
1553600866.443112: CookedMessage(header=CookedHeader(fixed=FixedHeader(endian=108, type=4, flags=1, version=1, length=76, cookie=6919136), fields=[Field(type=<MessageFieldType.PATH: 1>, data='/xyz/openbmc_project/sensors/fan_tach/fan0_0'), Field(type=<MessageFieldType.INTERFACE: 2>, data='org.freedesktop.DBus.Properties'), Field(type=<MessageFieldType.MEMBER: 3>, data='PropertiesChanged'), Field(type=<MessageFieldType.SIGNATURE: 8>, data='sa{sv}as'), Field(type=<MessageFieldType.SENDER: 7>, data=':1.95')]), body=['xyz.openbmc_project.Sensor.Value', [['Value', 3210]], []])

1553600866.456774: CookedMessage(header=CookedHeader(fixed=FixedHeader(endian=108, type=4, flags=1, version=1, length=76, cookie=6919137), fields=[Field(type=<MessageFieldType.PATH: 1>, data='/xyz/openbmc_project/sensors/fan_tach/fan1_0'), Field(type=<MessageFieldType.INTERFACE: 2>, data='org.freedesktop.DBus.Properties'), Field(type=<MessageFieldType.MEMBER: 3>, data='PropertiesChanged'), Field(type=<MessageFieldType.SIGNATURE: 8>, data='sa{sv}as'), Field(type=<MessageFieldType.SENDER: 7>, data=':1.95')]), body=['xyz.openbmc_project.Sensor.Value', [['Value', 3081]], []])

...
```

With JSON output, useful for piping through (`jq`)[https://stedolan.github.io/jq/]:
```
$ ./dbus-pcap --json | head -n 2
$ dbus-pcap --json dbus.pcap | head
[[[108, 4, 1, 1, 76, 6919136], [[1, "/xyz/openbmc_project/sensors/fan_tach/fan0_0"], [2, "org.freedesktop.DBus.Properties"], [3, "PropertiesChanged"], [8, "sa{sv}as"], [7, ":1.95"]]], ["xyz.openbmc_project.Sensor.Value", [["Value", 3210]], []]]
[[[108, 4, 1, 1, 76, 6919137], [[1, "/xyz/openbmc_project/sensors/fan_tach/fan1_0"], [2, "org.freedesktop.DBus.Properties"], [3, "PropertiesChanged"], [8, "sa{sv}as"], [7, ":1.95"]]], ["xyz.openbmc_project.Sensor.Value", [["Value", 3081]], []]]
...
```

## Discussion

While [Wireshark](https://www.wireshark.org/) has the ability to inspect D-Bus
captures it falls down in terms of scriptability and the filters exposed by the
dissector.

In addition to parsing and displaying packet contents `dbus-pcap` can filter
the capture based on [standard D-Bus match
expressions](https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-routing-match-rules)
(though does not yet support argument matching).
