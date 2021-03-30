# Intel IPMI OEM support library
This component is intended to provide Intel-specific IPMI`[3]` command handlers for
OpenBMC. These handlers are intended to integrate BMC with servers based on
Intel architecture.

## Overview
`intel-ipmi-oem` serves as an extension`[1]` to OpenBMC IPMI daemon`[2]`.
It is compiled as a shared library and intended to both:
- override existing implementation of standard IPMI commands to comply with
  Intel-specific solutions,
- provide implementation for non-standard OEM extensions.

## Capabilities
Related features provided by the library are grouped in separate source files.
Main extensions to vanilla OpenBMC IPMI stack are the following:
- Acquiring SMBIOS data over IPMI
- Commands for better integration with Intel hardware
- Firmware update extensions
- Extended parsing of IPMI Platform Events`[4]`

## References
1. [OpenBMC IPMI Architecture](https://github.com/openbmc/docs/blob/master/architecture/ipmi-architecture.md)
2. [Phosphor IPMI Host](https://github.com/openbmc/phosphor-host-ipmid)
3. [IPMI Specification v2.0](https://www.intel.pl/content/www/pl/pl/products/docs/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html)
4. [Intel Platform Events parsing](docs/Intel_IPMI_Platform_Events.md)