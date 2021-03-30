# IPZ VPD D-Bus Interfaces
IPZ is a VPD (Vital Product Data) format used in IBM Power systems.
The format consists of keywords that are stored as key-value
pairs (Keyword name and its value). Keywords are grouped into records,
usually with similar function grouped into a single record.

The [OpenPower VPD] [1] format is quite similar to the IPZ format
and describes the record-keyword structure.

Also refer to the [VPD Collection design document] [2] that describes how the
VPD collection application on the BMC will parse and publish the VPD data for
IBM Power systems on D-Bus.

The D-Bus interfaces defined here describe how IPZ VPD will be made available on
D-Bus. Each YAML here represents a record in the IPZ VPD and keywords that
belong to that record are represented as properties under that interface.
The type of every property shall be a byte array.

[1]:https://www-355.ibm.com/systems/power/openpower/posting.xhtml?postingId=1D060729AC96891885257E1B0053BC95
[2]:https://github.com/openbmc/docs/blob/master/designs/vpd-collection.md
