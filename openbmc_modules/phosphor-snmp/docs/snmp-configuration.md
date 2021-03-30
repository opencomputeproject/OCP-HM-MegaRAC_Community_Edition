# SNMP trap

phosphor-snmp currently only supports sending traps for error log entries.
Enabling that for a system can be done by adding phosphor-dbus-monitor rules.

There are a couple of ways through which the user can configure the Dbus-monitor
to generate the SNMP trap.

* Interface added signal on the D-Bus object.
* Properties changed signal on the D-Bus object.

Other OpenBMC applications can still send the SNMP trap as needed. These
applications must use the API exposed by the libsnmp.

Users need to add the notification object in the MIB and then application can
call the library function.

The OpenBMC Notification management information base (MIB) extension can be
found at the following location.
It defines the object descriptions needed for a management application to be
notified of an error entry in the OpenBMC log.

https://github.com/openbmc/phosphor-snmp/tree/master/mibs

The phosphor-snmp repository exposes the following lib and the D-Bus service

libsnmp: Exposes the API to send the SNMP trap
phosphor-network-snmpconf: Used for configuring the SNMP manager(Client).

# SNMP Manager Configuration

The administrator can configure one or more SNMP managers to receive the OpenBMC
traps.

## Add the SNMP Manager

### REST

```curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data": ["<snmp manager ip>", <port>] }' https://<hostname/ip>/xyz/openbmc_project/network/snmp/manager/action/Client```

### busctl

```busctl call xyz.openbmc_project.Network.SNMP /xyz/openbmc_project/network/snmp/manager xyz.openbmc_project.Network.Client.Create Client sqs "<snmp manager ip>" <port>```

## Delete the SNMP Manager

### REST

```curl -c cjar -b cjar -k -H "Content-Type: application/json" -X DELETE https://<hostname/ip>/xyz/openbmc_project/network/snmp/manager/<id>```

### busctl

```busctl call xyz.openbmc_project.Network.SNMP /xyz/openbmc_project/network/snmp/manager/<id> xyz.openbmc_project.Object.Delete Delete```

