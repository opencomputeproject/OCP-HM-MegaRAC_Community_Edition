# phosphor-sel-logger

### Overview

The SEL Logger daemon handles all requests to add new IPMI SEL records
to the journal.  SEL records stored in the journal are identified by
the standard **MESSAGE\_ID** metadata.  Other metadata fields are used to
store event-specific information for each record.

### Metadata

SEL records are identified in the journal using the **MESSAGE\_ID** field.

The **MESSAGE\_ID** for SEL records is **"b370836ccf2f4850ac5bee185b77893a"**.

The additional metadata fields for a SEL record are

    IPMI_SEL_RECORD_ID = Two byte unique SEL Record ID
    IPMI_SEL_RECORD_TYPE = The type of SEL entry (system or OEM)
                           which determines the definition of the
                           remaining bytes
    IPMI_SEL_GENERATOR_ID = The IPMI Generator ID (usually the
                            IPMB Slave Address) of the requester
    IPMI_SEL_SENSOR_PATH = D-Bus path of the sensor in the event
    IPMI_SEL_EVENT_DIR = Direction of the event (assert or deassert)
    IPMI_SEL_DATA = Raw binary data included in the SEL record

### Interface

The SEL Logger daemon exposes an interface for manually adding System
and OEM type SEL events, and provides the capability to monitor for
types of events and log them automatically.

The interface for System type events requires

* A text message to include in the journal entry
* The sensor path
* Up to three bytes of SEL data
* The direction of the event (assertion or deassertion)
* The generator ID of the requester

The interface for OEM type events requires

* A text message to include in the journal entry
* Up to thirteen bytes of SEL data (depending on the record type)
* The record type

The **MESSAGE\_ID** and **IPMI\_SEL\_RECORD\_ID** metadata fields are added by the
daemon.

### Event Monitoring

The SEL Logger daemon can be configured to watch for specific types of events and
automatically log SEL records for them.

As an example, the SEL Logger has a "threshold event monitor" which implements a
D-Bus match for any "PropertiesChanged" event on the
"xyz.openbmc_project.Sensor.Threshold" interface.  The handler then checks for any
new threshold events and logs SEL records accordingly.
