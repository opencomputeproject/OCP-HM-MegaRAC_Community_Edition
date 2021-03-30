# OpenPower Platform Event Log (PEL) extension

This extension will create PELs for every OpenBMC event log. It is also
possible to point to the raw PEL to use in the OpenBMC event, and then that
will be used instead of creating one.

## Contents
* [Passing in data when creating PELs](#passing-pel-related-data-within-an-openbmc-event-log)
* [Default UserData sections for BMC created PELs](#default-userdata-sections-for-bmc-created-pels)
* [The PEL Message Registry](#the-pel-message-registry)
* [Callouts](#callouts)
* [Action Flags and Event Type Rules](#action-flags-and-event-type-rules)
* [D-Bus Interfaces](#d-bus-interfaces)
* [PEL Retention](#pel-retention)

## Passing PEL related data within an OpenBMC event log

An error log creator can pass in data that is relevant to a PEL by using
certain keywords in the AdditionalData property of the event log.

### AdditionalData keywords

#### RAWPEL

This keyword is used to point to an existing PEL in a binary file that should
be associated with this event log.  The syntax is:
```
RAWPEL=<path to PEL File>
e.g.
RAWPEL="/tmp/pels/pel.5"
```
The code will assign its own error log ID to this PEL, and also update the
commit timestamp field to the current time.

#### ESEL

This keyword's data contains a full PEL in string format.  This is how hostboot
sends down PELs when it is configured in IPMI communication mode.  The PEL is
handled just like the PEL obtained using the RAWPEL keyword.

The syntax is:

```
ESEL=
"00 00 df 00 00 00 00 20 00 04 12 01 6f aa 00 00 50 48 00 30 01 00 33 00 00..."
```

Note that there are 16 bytes of IPMI SEL data before the PEL data starts.

#### _PID

This keyword that contains the application's PID is added automatically by the
phosphor-logging daemon when the `commit` or `report` APIs are used to create
an event log, but not when the `Create` D-Bus method is used.  If a caller of
the `Create` API wishes to have their PID captured in the PEL this should be
used.

This will be added to the PEL in a section of type User Data (UD), along with
the application name it corresponds to.

The syntax is:
```
_PID=<PID of application>
e.g.
_PID="12345"
```

#### CALLOUT_INVENTORY_PATH

This is used to pass in an inventory item to use as a callout.  See [here for
details](#passing-callouts-in-with-the-additionaldata-property)

#### CALLOUT_DEVICE_PATH with CALLOUT_ERRNO

This is used to pass in a device path to create callouts from.  See [here for
details](#passing-callouts-in-with-the-additionaldata-property)

#### CALLOUT_IIC_BUS with CALLOUT_IIC_ADDR and CALLOUT_ERRNO

This is used to pass in an I2C bus and address to create callouts from.  See
[here for details](#passing-callouts-in-with-the-additionaldata-property)

### FFDC Intended For UserData PEL sections

When one needs to add FFDC into the PEL UserData sections, the
`CreateWithFFDCFiles` D-Bus method on the `xyz.openbmc_project.Logging.Create`
interface must be used when creating a new event log. This method takes a list
of files to store in the PEL UserData sections.

That API is the same as the 'Create' one, except it has a new parameter:

```
std::vector<std::tuple<enum[FFDCFormat],
                       uint8_t,
                       uint8_t,
                       sdbusplus::message::unix_fd>>
```

Each entry in the vector contains a file descriptor for a file that will
be stored in a unique UserData section.  The tuple's arguments are:

- enum[FFDCFormat]: The data format type, the options are:
    - 'JSON'
        - The parser will use nlohmann::json\'s pretty print
    - 'CBOR'
        - The parser will use nlohmann::json\'s pretty print
    - 'Text'
        - The parser will output ASCII text
    - 'Custom'
        - The parser will hexdump the data, unless there is a parser registered
          for this component ID and subtype.
- uint8_t: subType
  - Useful for the 'custom' type.  Not used with the other types.
- uint8_t: version
  - The version of the data.
  - Used for the custom type.
  - Not planning on using for JSON/BSON unless a reason to do so appears.
- unixfd - The file descriptor for the opened file that contains the
    contents.  The file descriptor can be closed and the file can be deleted if
    desired after the method call.

An example of saving JSON data to a file and getting its file descriptor is:

```
nlohmann::json json = ...;
auto jsonString = json.dump();
FILE* fp = fopen(filename, "w");
fwrite(jsonString.data(), 1, jsonString.size(), fp);
int fd = fileno(fp);
```

Alternatively, 'open()' can be used to obtain the file descriptor of the file.

Upon receiving this data, the PEL code will create UserData sections for each
entry in that vector with the following UserData fields:

- Section header component ID:
    - If the type field from the tuple is "custom", use the component ID from
      the message registry.
    - Otherwise, set the component ID to the phosphor-logging component ID so
      that the parser knows to use the built in parsers (e.g. json) for the
      type.
- Section header subtype: The subtype field from the tuple.
- Section header version: The version field from the tuple.
- Section data: The data from the file.

If there is a peltool parser registered for the custom type (method is TBD),
that will be used by peltool to print the data, otherwise it will be hexdumped.

Before adding each of these UserData sections, a check will be done to see if
the PEL size will remain under the maximum size of 16KB.  If not, the UserData
section will be truncated down enough so that it will fit into the 16KB.

## Default UserData sections for BMC created PELs

The extension code that creates PELs will add these UserData sections to every
PEL:

- The AdditionalData property contents
  - If the AdditionalData property in the OpenBMC event log has anything in it,
    it will be saved in a UserData section as a JSON string.

- System information
  - This section contains various pieces of system information, such as the
    full code level and the BMC, chassis, and host state properties.

## The PEL Message Registry

The PEL message registry is used to create PELs from OpenBMC event logs.
Documentation can be found [here](registry/README.md).

## Callouts

A callout points to a FRU, a symbolic FRU, or an isolation procedure.  There
can be from zero to ten of them in each PEL, where they are located in the SRC
section.

There are a few different ways to add callouts to a PEL:

### Passing callouts in with the AdditionalData property

The PEL code can add callouts based on the values of special entries in the
AdditionalData event log property.  They are:

- CALLOUT_INVENTORY_PATH

    This keyword is used to call out a single FRU by passing in its D-Bus
    inventory path.  When the PEL code sees this, it will create a single high
    priority FRU callout, using the VPD properties (location code, FN, CCIN)
    from that inventory item.  If that item is not a FRU itself and does not
    have a location code, it will keep searching its parents until it finds one
    that is.

    ```
    CALLOUT_INVENTORY_PATH=
    "/xyz/openbmc_project/inventory/system/chassis/motherboard"
    ```

- CALLOUT_DEVICE_PATH with CALLOUT_ERRNO

    These keywords are required as a pair to indicate faulty device
    communication, usually detected by a failure accessing a device at that
    sysfs path.  The PEL code will use a data table generated by the MRW to map
    these device paths to FRU callout lists.  The errno value may influence the
    callout.

    I2C, FSI, FSI-I2C, and FSI-SPI paths are supported.

    ```
    CALLOUT_DEVICE_PATH="/sys/bus/i2c/devices/3-0069"
    CALLOUT_ERRNO="2"
    ```

- CALLOUT_IIC_BUS with CALLOUT_IIC_ADDR and CALLOUT_ERRNO

    These 3 keywords can be used to callout a failing I2C device path when the
    full device path isn't known.  It is similar to CALLOUT_DEVICE_PATH in that
    it will use data tables generated by the MRW to create the callouts.

    CALLOUT_IIC_BUS is in the form "/dev/i2c-X" where X is the bus number, or
    just the bus number by itself.
    CALLOUT_IIC_ADDR is the 7 bit address either as a decimal or a hex number
    if preceded with a "0x".

    ```
    CALLOUT_IIC_BUS="/dev/i2c-7"
    CALLOUT_IIC_ADDR="81"
    CALLOUT_ERRNO=62
    ```

### Defining callouts in the message registry

Callouts can be completely defined inside that error's definition in the PEL
message registry.  This method allows the callouts to vary based on the system
type or on any AdditionalData item.

At a high level, this involves defining a callout section inside the registry
entry that contain the location codes or procedure names to use, along with
their priority.  If these can vary based on system type, the type provided by
the entity manager will be one of the keys.  If they can also depend on an
AdditionalData entry, then that will also be a key.

See the message registry [README](registry/README.md) and
[schema](registry/schema/schema.json) for the details.

### Using the message registry along with CALLOUT_ entries

If the message registry entry contains a callout definition and the event log
also contains one of aforementioned CALLOUT keys in the AdditionalData
property, then the PEL code will first add the callouts stemming from the
CALLOUT items, followed by the callouts from the message registry.

### Using external callout tables

Some applications, such as the code from the openpower-hw-diags repository,
have their own callout tables that contain the callouts to use for the errors
they generate.

**TODO**:  The best way to pass these callouts in to the PEL creation code.

## `Action Flags` and `Event Type` Rules

The `Action Flags` and `Event Type` PEL fields are optional in the message
registry, and if not present the code will set them based on certain rules
layed out in the PEL spec.  In fact, even if they were specified, the checks
are still done to ensure consistency across all the logs.

These rules are:
1. Always set the `Report` flag, unless the `Do Not Report` flag is already on.
2. Always clear the `SP Call Home` flag, as that feature isn't supported.
3. If the severity is `Non-error Event`:
    - Clear the `Service Action` flag.
    - Clear the `Call Home` flag.
    - If the `Event Type` field is `Not Applicable`, change it to `Information
      Only`.
    - If the `Event Type` field is `Information Only` or `Tracing`, set the
      `Hidden` flag.
4. If the severity is `Recovered`:
    - Set the `Hidden` flag.
    - Clear the `Service Action` flag.
    - Clear the `Call Home` flag.
5. For all other severities:
    - Clear the `Hidden` flag.
    - Set the `Service Action` flag.
    - Set the `Call Home` flag.

Additional rules may be added in the future if necessary.

## D-Bus Interfaces

See the org.open_power.Logging.PEL interface definition for the most up to date
information.

## PEL Retention

The PEL repository is allocated a set amount of space on the BMC.  When that
space gets close to being full, the code will remove a percentage of PELs to
make room for new ones.  In addition, the code will keep a cap on the total
number of PELs allowed.  Note that removing a PEL will also remove the
corresponding OpenBMC event log.

The disk capacity limit is set to 20MB, and the number limit is 3000.

The rules used to remove logs are listed below.  The checks will be run after a
PEL has been added and the method to create the PEL has returned to the caller,
i.e. run when control gets back to the event loop.

### Removal Algorithm

If the size used is 95% or under of the allocated space and under the limit on
the number of PELs, nothing further needs to be done, otherwise continue and
run all 5 of the following steps.  Each step itself only deletes PELs until it
meets its requirement and then it stops.

The steps are:

1. Remove BMC created informational PELs until they take up 15% or less of the
   allocated space.

2. Remove BMC created non-informational PELs until they take up 30% or less of
   the allocated space.

3. Remove non-BMC created informational PELs until they take up 15% or less of
   the allocated space.

4. Remove non-BMC created non-informational PELs until they take up 30% or less
   of the allocated space.

5. After the previous 4 steps are complete, if there are still more than the
   maximum number of PELs, remove PELs down to 80% of the maximum.

PELs with associated guard records will never be deleted.  Each step above
makes the following 4 passes, stopping as soon as its limit is reached:

Pass 1. Remove HMC acknowledged PELs.<br>
Pass 2. Remove OS acknowledged PELs.<br>
Pass 3. Remove PHYP acknowledged PELs.<br>
Pass 4. Remove all PELs.

After all these steps, disk capacity will be at most 90% (15% + 30% + 15% +
30%).
