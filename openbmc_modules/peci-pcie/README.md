# peci-pcie

The peci-pcie application uses the CPU PECI interface to get PCIe Device
information for the system and shares it on D-Bus.  The information from D-Bus
is used by bmcweb to populate the PCIe Device resources in Redfish.
