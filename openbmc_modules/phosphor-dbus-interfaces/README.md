# phosphor-dbus-interfaces
YAML descriptors of standard D-Bus interfaces.
The format is described by the [sdbusplus binding generation tool sdbus++][].

Only the xyz/openbmc_project interfaces are built by default.

Enable the OpenPower D-Bus interfaces with:
```
--enable-openpower-dbus-interfaces
```
Enable IBM D-Bus interfaces with:
```
--enable-ibm-dbus-interfaces
```

[sdbusplus binding generation tool sdbus++]: https://github.com/openbmc/sdbusplus/blob/master/README.md#binding-generation-tool
