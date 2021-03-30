# My First Sensors

This document is meant to bring you from nothing to using Entity-Manager with
Dbus-Sensors to populate a plug-in card's sensor values on dbus. Once the sensor
values are on dbus, they can be read via IPMI or Redfish, and this is beyond
this guide.

For the sake of this very simple example, let's pretend there is a PCIe card
that exposes an 24c02 eeprom and a tmp441 sensor. The PCIe slots are behind an
smbus mux on the motherboard and are in a device-tree such as this:

```
aliases {
        i2c16 = &i2c_pe0;
        i2c17 = &i2c_pe1;
        i2c18 = &i2c_pe2;
        i2c19 = &i2c_pe3;
};

...

&i2c1 {
    status = "okay";
    i2c-switch@71 {
        compatible = "nxp,pca9546";
        reg = <0x71>;
        #address-cells = <1>;
        #size-cells = <0>;
        i2c-mux-idle-disconnect;

        i2c_pe0: i2c@0 {
            #address-cells = <1>;
            #size-cells = <0>;
            reg = <0>;
        };
        i2c_pe1: i2c@1 {
            #address-cells = <1>;
            #size-cells = <0>;
            reg = <1>;
        };
        i2c_pe2: i2c@2 {
            #address-cells = <1>;
            #size-cells = <0>;
            reg = <2>;
        };
        i2c_pe3: i2c@3 {
            #address-cells = <1>;
            #size-cells = <0>;
            reg = <3>;
        };
    };
};
```

The first daemon of interest that will run is the FruDevice portion of
Entity-Manager. The exact layout of a FRU is beyond this guide, but for this
example the PCIe card's eeprom holds the following information:

```
Product:
  MANUFACTURER  "Awesome"
  PART_NUMBER   "12345"
  PRODUCT_NAME  "Super Great"
  SERIAL_NUMBER "12312490840"
```

The FruDevice daemon will walk all i2c buses and attempt to find FRU contents at
responsive smbus addresses. When if finds a FRU it will parse the contents and
publish them to dbus keying off the most prominent name field. In this case, it
found two of the cards. One at bus 18 and the other at 19.

The dbus tree for this will look like: ```

```
~# busctl tree --no-pager xyz.openbmc_project.FruDevice
`-/xyz
  `-/xyz/openbmc_project
    `-/xyz/openbmc_project/FruDevice
      |-/xyz/openbmc_project/FruDevice/Super_Great
      |-/xyz/openbmc_project/FruDevice/Super_Great_0
```

The dbus path for each instance is unimportant beyond needing to be unique.
Digging into one of these FRUs we see:

```
~# busctl introspect --no-pager xyz.openbmc_project.FruDevice \
 /xyz/openbmc_project/FruDevice/Super_Great

NAME                                TYPE      SIGNATURE RESULT/VALUE                FLAGS
org.freedesktop.DBus.Introspectable interface -         -                           -
.Introspect                         method    -         s                           -
org.freedesktop.DBus.Peer           interface -         -                           -
.GetMachineId                       method    -         s                           -
.Ping                               method    -         -                           -
org.freedesktop.DBus.Properties     interface -         -                           -
.Get                                method    ss        v                           -
.GetAll                             method    s         a{sv}                       -
.Set                                method    ssv       -                           -
.PropertiesChanged                  signal    sa{sv}as  -                           -
xyz.openbmc_project.FruDevice       interface -         -                           -
.ADDRESS                            property  u         80                          emits-change
.BUS                                property  u         18                          emits-change
.Common_Format_Version              property  s         "1"                         emits-change
.PRODUCT_ASSET_TAG                  property  s         "--"                        emits-change
.PRODUCT_FRU_VERSION_ID             property  s         "??????"                    emits-change
.PRODUCT_LANGUAGE_CODE              property  s         "0"                         emits-change
.PRODUCT_MANUFACTURER               property  s         "Awesome"                   emits-change
.PRODUCT_PART_NUMBER                property  s         "12345"                     emits-change
.PRODUCT_PRODUCT_NAME               property  s         "Super Great"               emits-change
.PRODUCT_SERIAL_NUMBER              property  s         "12312490840"               emits-change
.PRODUCT_VERSION                    property  s         "0A"                        emits-change
```

Ok, now you can find the cards, but what about the temperature sensors on each
of them? Entity-Manager provides a very powerful mechanism for querying various
information, but our goal is simple. If we find the card, we want to add the
device to the system and tell dbus-sensors that there is a hwmon temperature
sensor available.

We start with a simple hardware profile. We know that if the card's bus is
identified we know the address of the temperature sensor is 0x4c.

```
{
    "Exposes": [
       {
            "Address": "$address",
            "Bus": "$bus",
            "Name": "$bus great eeprom",
            "Type": "24C02"
        },
        {
            "Address": "0x4c",
            "Bus": "$bus",
            "Name": "$bus great local",
            "Name1": "$bus great ext",
            "Type": "TMP441"
        }
    ],
    "Name": "$bus Great Card",
    "Probe": "xyz.openbmc_project.FruDevice({'PRODUCT_PRODUCT_NAME': 'Super Great'})",
    "Type": "Board"
}
```

There's a lot going on in the above hardware profile, and they can become
considerably more complex. Firstly, let's start with the `Probe` field. This is
a way of defining under what circumstances this hardware profile is applied. In
this case, we want the hardware profile to be applied when a Fru is found with
the field `PRODUCT_PRODUCT_NAME` holding the value `Super Great`. In our system,
this will match twice. When the probe has matched the information from that
device is then swapped into the hardware profile via the templated variables,
such as `$bus` or `$address`. We then shift our focus to the `Exposes` field.
This lists the entities that are added when this hardware profile is loaded. The
field is optional and there is a wide variety of entities that can be added this
way.

In our example we only care about the eeprom and the temperature sensor. The
`Type` field is checked against a device export map and if it matches a known
device, it'll attempt to install the device.

For the card found on bus 18:

```
echo "24c02 0x50 > /sys/bus/i2c/devices/i2c-18/new_device"
echo "tmp441 0x4c > /sys/bus/i2c/devices/i2c-18/new_device"
```

Beyond this, it also publishes to dbus a configuration:

```
~# busctl tree --no-pager xyz.openbmc_project.EntityManager
`-/xyz
  `-/xyz/openbmc_project
    |-/xyz/openbmc_project/EntityManager
    `-/xyz/openbmc_project/inventory
      `-/xyz/openbmc_project/inventory/system
        `-/xyz/openbmc_project/inventory/system/board
          |-/xyz/openbmc_project/inventory/system/board/18_Great_Card
          | |-/xyz/openbmc_project/inventory/system/board/18_Great_Card/18_great_local
          |-/xyz/openbmc_project/inventory/system/board/19_Great_Card
          | |-/xyz/openbmc_project/inventory/system/board/19_Great_Card/19_great_local

~# busctl introspect --no-pager xyz.openbmc_project.EntityManager \
 /xyz/openbmc_project/inventory/system/board/18_Great_Card/18_great_local

NAME                                     TYPE      SIGNATURE RESULT/VALUE    FLAGS
org.freedesktop.DBus.Introspectable      interface -         -               -
.Introspect                              method    -         s               -
org.freedesktop.DBus.Peer                interface -         -               -
.GetMachineId                            method    -         s               -
.Ping                                    method    -         -               -
org.freedesktop.DBus.Properties          interface -         -               -
.Get                                     method    ss        v               -
.GetAll                                  method    s         a{sv}           -
.Set                                     method    ssv       -               -
.PropertiesChanged                       signal    sa{sv}as  -               -
xyz.openbmc_project.Configuration.TMP441 interface -         -               -
.Address                                 property  t         76               emits-change
.Bus                                     property  t         18               emits-change
.Name                                    property  s         "18 great local" emits-change
.Name1                                   property  s         "18 great ext"   emits-change
.Type                                    property  s         "TMP441"         emits-change
```

The dbus-sensors suite of daemons each run searching for a specific type of
sensor. In this case the hwmon temperature sensor daemon will recognize the
configuration interface: `xyz.openbmc_project.Configuration.TMP441`.

It will look up the device on i2c and see there is a hwmon instance, and map
`temp1_input` to `Name` and since there is also `Name1` it'll map `temp2_input`.

```
~# busctl tree --no-pager Service xyz.openbmc_project.HwmonTempSensor

root@semitruck:~# busctl tree --no-pager xyz.openbmc_project.HwmonTempSensor
`-/xyz
  `-/xyz/openbmc_project
    `-/xyz/openbmc_project/sensors
      `-/xyz/openbmc_project/sensors/temperature
        |-/xyz/openbmc_project/sensors/temperature/18_great_local
        |-/xyz/openbmc_project/sensors/temperature/18_great_ext
        |-/xyz/openbmc_project/sensors/temperature/19_great_local
        |-/xyz/openbmc_project/sensors/temperature/19_great_ext

~# busctl introspect --no-pager xyz.openbmc_project.HwmonTempSensor \
 /xyz/openbmc_project/sensors/temperature/18_great_local

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
org.openbmc.Associations            interface -         -                                        -
.associations                       property  a(sss)    1 "chassis" "all_sensors" "/xyz/openb... emits-change
xyz.openbmc_project.Sensor.Value    interface -         -                                        -
.MaxValue                           property  d         127                                      emits-change
.MinValue                           property  d         -128                                     emits-change
.Value                              property  d         31.938                                   emits-change writable

```

There you are! You now have the two sensors from the two card instances on dbus.

This can be more complex, for instance if your card has a mux you can add this,
will trigger FruDevice to scan those new buses for more devices.
