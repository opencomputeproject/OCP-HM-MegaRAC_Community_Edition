On platforms running AMI BMC firmware (Habanero and Firestone at the moment),
the 'Get/Set' System Boot Options' command has been extended to allow overrides
to the network configuration to be specified.
In Petitboot this will cause any existing network configuration to be overridden
by the specified configuration. The format of each IPMI request is similar to
that of the usual boot options request, with a slightly different payload.

The start of the request is similar to a usual boot option override, but
specifies the fields of interest differently, ie;

Specify 'chassis bootdev', field 96, data1
0x00 0x08 0x61 0x80

The rest of request format is defined by Petitboot as:
    - 4 byte cookie value (always 0x21 0x70 0x62 0x21)
    - 2 byte version value (always 0x00 0x01)
    - 1 byte hardware address size (eg. 0x06 for MAC address)
    - 1 byte IP address size (eg. 0x04 for IPv4)
    - Hardware (MAC) address
    - 1 byte flags for 'ignore' and 'method', where method = 0 is DHCP
      and method = 1 is Static.
And for static configs:
    - IP Address
    - 1 byte subnet value
    - Gateway address

Describing each field in more detail:

Specify 'chassis bootdev', field 96, data1
0x00 0x08 0x61 0x80

Set a special cookie that Petitboot will recognise:
0x21 0x70 0x62 0x21

Specify the version (only 1 at the moment)
0x00 0x01

Specify the size of the MAC address and IP address. This is used to
differentiate between IPv4 and IPv6 addresses, or potential future support for
Infiniband/etc.
0x06 0x04   (6-byte MAC address, IPv4 IP address)

Set the hardware address of the interface you want to override, eg:
0xf4 0x52 0x14 0xf3 0x01 0xdf

Specify 'ignore' or 'static/dynamic' flags. The second byte specifies to use
either DHCP or static IP configuration (0 for DHCP).
0x00 0x01

The below fields are required if setting a static configuration:

Set the IP address you want to use, eg:
0x0a 0x3d 0xa1 0x42

Set the subnet mask (short notation), eg '16':
0x10

Set the gateway address, eg:
0x0a 0x3d 0x2 0x1

All together this should look like:
0x00 0x08 0x61 0x80 0x21 0x70 0x62 0x21
0x00 0x01 0x06 0x04 0xf4 0x52 0x14 0xf3
0x01 0xdf 0x00 0x01 0x0a 0x3d 0xa1 0x42
0x10 0x0a 0x3d 0x2 0x1

To clear a network override, it is sufficient to clear out the request, or set
a zero-cookie which Petitboot will reject. Eg:
0x00 0x08 0x61 0x80 0x00 0x00 0x00 0x00

You can 'Get' the override back with
0x00 0x09 0x61 0x80 0x00
which should return whatever is currently set.

