# Network Management

## Overview

A Network Manager is a daemon which handles network management operations.
It must implement the `xyz.openbmc_project.Network.SystemConfiguration.interface`
and `org.freedesktop.DBus.ObjectManager`.

When the network manager daemon comes up, it should create objects
implementing physical link/virtual interfaces such as
`xyz.openbmc_project.Network.EthernetInterface` or
`xyz.openbmc_project.Network.VLANInterface` on the system.

IP address(v4 and v6) objects must be children objects of the
physical/virtual interface object.

## Interfaces

1. SystemConfiguration: This describes the system-specific parameters.
2. EthernetInterface: This describes the interface specific parameters.
3. IP: This describes the IP address specific parameters.
4. IPProtocol: This describes the IP protocol type(IPv4/IPv6).
5. VLANInterface: This describes the VLAN specific properties.
6. Bond: This describes the interface bonding parameters.

## D-Bus Objects

#### Interface Objects

Interface objects can be physical as well as virtual.

If the object is a physical interface, it can't be deleted,
but if it is a virtual interface object it can be deleted.

E.g. `/xyz/openbmc_project/network/<interfacename>`

#### IP Address Objects

There can be multiple IP address objects under an interface object.
These objects can be deleted by the delete function.

IPv4 objects will have the following D-Bus object path:

`/xyz/openbmc_project/network/<interface>/ipv4/<id>`

IPv6 objects will have the following D-Bus object path:

`/xyz/openbmc_project/network/<interface>/ipv6/<id>`

#### Network Configuration Object

The network configuration object will have system configuration parameters:

`/xyz/openbmc_project/network/conf`

## Commands

#### Create Static IPv4 Address

```
busctl call  xyz.openbmc_project.Network /xyz/openbmc_project/network/<interface> xyz.openbmc_project.Network.IP.Create IP ssys "xyz.openbmc_project.Network.IP.Protocol.IPv4" "<IP Address>" <Netmask Prefix> "<Network Gateway>"
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X  POST -d '{"data":["xyz.openbmc_project.Network.IP.Protocol.IPv4","<IP Address>", <Netmask Prefix>, "<Network Gateway>"]
}' https://${bmc}/xyz/openbmc_project/network/<interface>/action/IP
```

E.g.
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X  POST -d '{"data":["xyz.openbmc_project.Network.IP.Protocol.IPv4","8.8.8.8", 24, "8.8.8.0"]}' https://${bmc}/xyz/openbmc_project/network/eth0/action/IP
```

Note: After creating the IP address object enumerate the network interface object to get the IPv4 id.

#### Delete IPv4 Address

```
busctl call xyz.openbmc_project.Network /xyz/openbmc_project/network/<interface>/ipv4/<id> xyz.openbmc_project.Object.Delete Delete
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/network/<interface>/ipv4/<id>
```

#### Default Gateway

##### Get

```
busctl get-property xyz.openbmc_project.Network /xyz/openbmc_project/network/config xyz.openbmc_project.Network.SystemConfiguration DefaultGateway
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" https://${bmc}/xyz/openbmc_project/network/config/attr/DefaultGateway
```

##### Set

```
busctl set-property xyz.openbmc_project.Network /xyz/openbmc_project/network/config xyz.openbmc_project.Network.SystemConfiguration DefaultGateway s "<DefaultGateway>"
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X  PUT  -d '{"data": "<DefaultGateway>"}' https://${bmc}/xyz/openbmc_project/network/config/attr/DefaultGateway
```

NOTE: The default gateway must be pingable, if not 0.0.0.0 will be used.

#### HostName

##### Get

```
busctl get-property xyz.openbmc_project.Network /xyz/openbmc_project/network/config xyz.openbmc_project.Network.SystemConfiguration HostName
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" https://${bmc}/xyz/openbmc_project/network/config/attr/HostName
```

##### Set

```
busctl set-property xyz.openbmc_project.Network /xyz/openbmc_project/network/config xyz.openbmc_project.Network.SystemConfiguration HostName s "<HostName>"
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X  PUT  -d '{"data": "<HostName>"}' https://${bmc}/xyz/openbmc_project/network/config/attr/HostName
```

#### DHCP

##### Get

```
busctl get-property xyz.openbmc_project.Network /xyz/openbmc_project/network/eth0 xyz.openbmc_project.Network.EthernetInterface DHCPEnabled
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" https://${bmc}/xyz/openbmc_project/network/eth0/attr/DHCPEnabled
```

##### Enable

```
busctl set-property xyz.openbmc_project.Network /xyz/openbmc_project/network/eth0 xyz.openbmc_project.Network.EthernetInterface DHCPEnabled b 1
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X  PUT  -d '{"data": 1}' https://${bmc}/xyz/openbmc_project/network/eth0/attr/DHCPEnabled
```

#### MAC Address

##### Get

```
busctl get-property xyz.openbmc_project.Network /xyz/openbmc_project/network/eth0 xyz.openbmc_project.Network.MACAddress MACAddress
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" https://${bmc}/xyz/openbmc_project/network/<interface>/attr/MACAddress
```

##### Set

```
busctl set-property xyz.openbmc_project.Network /xyz/openbmc_project/network/<interface> xyz.openbmc_project.Network.MACAddress MACAddress s "<MAC Address>"
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/jon" -X  PUT  -d '{"data": "<MAC Address>" }' https://${bmc}/xyz/openbmc_project/network/<interface>/attr/MACAddress
```

NOTE: MAC address should be a local admin MAC (2nd bit of first byte should be on).

#### Network Factory Reset

```
busctl call xyz.openbmc_project.Network /xyz/openbmc_project/network xyz.openbmc_project.Common.FactoryReset Reset
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data":[] }' https://${bmc}/xyz/openbmc_project/network/action/Reset
```

#### VLAN

##### Create

```
busctl call xyz.openbmc_project.Network /xyz/openbmc_project/network xyz.openbmc_project.Network.VLAN.Create VLAN su "<interface>" <VLAN id>
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data":["<interface>", <VLAN id>] }' https://${bmc}/xyz/openbmc_project/network/action/VLAN
```

E.g.
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data":["eth0",50] }' https://${bmc}/xyz/openbmc_project/network/action/VLAN
```

##### Delete

```
busctl call xyz.openbmc_project.Network /xyz/openbmc_project/network/<VLAN interface> xyz.openbmc_project.Object.Delete Delete
```
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/network/<VLAN interface>
```

E.g.
```
curl -c cjar -b cjar -k -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/network/eth0_50
```

##### Enumerate

```
curl -c cjar -b cjar -k -H "Content-Type: application/json" https://${bmc}/xyz/openbmc_project/network/<VLAN interface>/enumerate
```

#### IPMI VLAN and IP

#####  Create

```
ipmitool -I dbus lan set 1 ipsrc static

ipmitool -I dbus lan set 1 ipaddr <IP address>

ipmitool -I dbus lan set 1 netmask <mask>

ipmitool -I dbus lan set 1 defgw ipaddr <IP address>

ipmitool -I dbus lan set 1 vlan id <id>

ipmitool -I dbus raw 0x06 0x40 // To the save settings
```

NOTE: It takes 4-5 seconds to create the VLAN and configure the IP.
If a VLAN interface is not desired don't set the VLAN id above.

##### Delete

```
ipmitool -I dbus lan set 1 vlan id off

ipmitool -I dbus raw 0x06 0x40 // To the save settings
```
