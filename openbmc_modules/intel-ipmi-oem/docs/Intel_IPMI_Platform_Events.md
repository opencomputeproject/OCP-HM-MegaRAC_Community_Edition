# Intel IPMI Platform Events parsing
In many cases Manufacturers-specific IPMI Platfrom Events are stored in binary
form in System Event Log making it very difficult to easily understand platfrom
state. This document specifies a solution for presenting Manufacturer Spcific
IPMI Platform Events in a human readable form by defining a generic framework
for parsing and defining new messages in an easy and scallable way.
Example of events originating from Intel Management Engine (ME) is used as
a case-study. General design of the solution is followed by tailored-down
implementation for OpenBMC described in detail.

## Glossary
- **IPMI** - Intelligent Platform Management Interface; standarized binary
    protocol of communication between endpoints in datacenter `[1]`
- **Platform Event** - specific type of IPMI binary payload, used for encoding
    and sending asynchronous one-way messages to recipient `[1]-29.3`
- **ME** - Intel Management Engine, autonomous subsystem used for remote
    datacenter management `[5]`
- **Redfish** - modern datacenter management protocol, built around REST
    protocol and JSON format `[2]`
- **OpenBMC** - open-source BMC implementation with Redfish-oriented
    interface `[3]`


## Problem statement
IPMI is designed to be a compact and efficient binary format of data exchanged
between entities in data-center. Recipient is responsible to receive data,
properly analyze, parse and translate the binary representation to
human-readable format. IPMI Platform Events is one type of these messages,
used to inform recipient about occurence of a particular well defined situation.

Part of IPMI Platform Events are standarized and described in the specification
and already have an open-source implementation ready `[6]`, however this is only
part of the spectrum. Increasing complexity of datacenter systems have multipled
possible sources of events which are defined by manufacturer-specirfic
extenstions to platform event data. One of these sources is Intel ME, which
is able to deliver information about its own state of operation and in some
cases notify about certain erroneous system-wide conditions, like interface
errors.

These OEM-specific messages lacks support in existing open-source
implementations. They require manual, documentation-based `[5]` implementation,
which is historically the source of many interpretation errors. Any document
update requires manual code modification according to specific changes which is
not efficient nor scalable. Furthermore - documentation is not always
clear on event severity or possible resolution actions.

## Solution
Generic OEM-agnostic algorithm is proposed to achieve human-readable output
for binary IPMI Platform Event.

In general, each event consists of predefined payload:
```ascii
[GeneratorID][SensorNumber][EventType][EventData[2]]
```
where:
- `GeneratorID` - used to determine source of the event,
- `SensorNumber` - generator-specific unique sensor number,
- `EventType` - sensor-specific group of events,
- `EventData` - array with detailed event data.

One might observe, that each consecutive event field is narrowing down the
domain of event interpretations, starting with `GeneratorID` at the top, ending
with `EventData` at the end of a `decision tree`. Software should be able
to determine meaning of the event by using the `divide and conquer` approach
for predefined list of well known event definitions. One should notice the fact,
that such decision tree might be also needed for breakdown of `EventData`,
as in many OEM-specific IPMI implementations that is also the case.

Implementation should be therefore a series of filters with increasing
specialization on each level. Recursive algorithm for this will look like
the following:
```ascii
     +-------------+           +*Step 1*               +
     | +---------+ |           |                       |
     | |Currently| |           |Analyze and choose     |
+----> |analyzed +------------>+proper 'subtree' parser|
|    | |chunk    | |           |                       |
|    | +---------+ |           +                       +         +---------+
|    | +---------+ |                                             |Remainder|
|    | |Remainder| |                                             |         |
|    | |         | |           +*Step 2*               +         |         |
|    | |         | |           |                       |         |         |
|    | |         +---------------------------------------------->+         +---+
|    | |         | |           |'Cut' the remainder    |         |         |   |
|    | |         | |           |and go back to Step 1  |         |         |   |
|    | |         | |           +                       +         |         |   |
|    | +---------+ |                                             |         |   |
|    +-------------+                                             +---------+   |
|                                                                              |
|                                                                              |
+------------------------------------------------------------------------------+
```
Described process will be repeated until there is nothing to break-down and
singular unique event interpretation will be determined (an `EventId`).

Not all event data is a decision point - certain chunks of data should be kept
as-is or formatted in certain way, to be introduced in human-readable `Message`.
Parser operation should also include a logic for extracting  `Parameters` during the traversal process.

Effectively, both `EventId` and an optional collection of `Parameters` should be
then used as input for lookup mechanic to generate final `Event Message`.
Each message consists of following entries:
- `EventId` - associated unique event,
- `Severity` - determines how severely this particular event might affect usual
    datacenter operation,
- `Resolution` - suggested steps to mitigate possible problem,
- `Message` - human-readable message, possibly with predefined placeholders for
    `Parameters`.

### Example
Example of such message parsing process is shown below:
```ascii
        +-------------+
        |[GeneratorId]|
        |0x2C (ME)    |
        +------+------+
               |
        +------v---------+
        |[SensorNumber]  |
. . . . |0x17 (ME Health)|
        +------+---------+
               |
        +------v---------+
        |[EventType]     |
. . . . |0x00 (FW Status)|
        +------+---------+
               |
        +------v-------------------+
        |[EventData[0]]            |           +-------------------------------------------+
. . . . |0x0A (FlashWearoutWarning)+------+    |ParsedEvent|                               |
        +------+-------------------+      |    +-----------+                               |
               |                          +---->'EventId' = FlashWearoutWarning            |
        +------v----------+               +---->'Parameters' = [ toDecimal(EventData[1]) ] |
        |[EventData[1]]   |               |    |                                           |
        |0x## (Percentage)+---------------+    +-------------------------------------------+
        +-----------------+
```
, determined `ParsedEvent` might be then passed to lookup mechanism,
which contains human-readable information for each `EventId`:
```ascii
+------------------------------------------------+
|+------------------------------------------------+
||+------------------------------------------------+
||| EventId: FlashWearoutWarning                   |
||| Severity: Warning                              |
||| Resolution: No immediate repair action needed  |
||| Message: Warning threshold for number of flash |
|||          operations has been exceeded. Current |
|||          percentage of write operations        |
+||          capacity: %1                          |
 +|                                                |
  +------------------------------------------------+

```

## Solution in OpenBMC
Proposed algorithm is delivered as part of open-source OpenBMC project `[3]`.
As this software stack is built with micro-service architecture in mind,
the implementation had to be divided into multiple parts:
- IPMI Platform Event payload unpacking  (`[7]`)
  - `openbmc/intel-ipmi-oem/src/sensorcommands.cpp`
  - `openbmc/intel-ipmi-oem/src/ipmi_to_redfish_hooks.cpp`
- Intel ME event parsing
  - `openbmc/intel-ipmi-oem/src/me_to_redfish_hooks.cpp`
- Detected events storage (`[4]`)
  - `systemd journal`
- Human-readable message lookup (`[2], [8]`)
  - `MessageRegistry in bmcweb`
    - `openbmc/bmcweb/redfish-core/include/registries/openbmc_message_registry.hpp`

### OpenBMC flow
#### Event arrival
1. IPMI driver notifies `intel-ipmi-oem` about incoming `Platform Event`
    (NetFn=0x4, Cmd=0x2)
   - Proper command handler in `intel-ipmi-oem/src/sensorcommands.cpp`
        is notified
2. Message is forwarded to `intel-ipmi-oem/src/ipmi_to_redfish_hooks.cpp`
    as call to `sel::checkRedfishHooks`
    - `sel::checkRedfishHooks` analyzes the data, `BIOS` events are handled
        in-place, while `ME` events are delegated to `intel-ipmi-oem/src/me_to_redfish_hooks.cpp`
3. `me::messageHook` is called with the payload. Parsing algorithm
    determines final `EventId` and `Parameters`
    - `me::utils::storeRedfishEvent(EventId, Parameters)` is called,
        it stores event securely in `system journal`

#### Platform Event payload parsing
Each IPMI Platform Event is parsed using aforementioned `me::messageHook`
handler. Implementation of the proposed algorithm is the following:

##### 1. Determine EventType
Based on `EventType` proper designated handler is called.
```cpp
namespace me {
static bool messageHook(const SELData& selData, std::string& eventId,
                        std::vector<std::string>& parameters)
{
    const HealthEventType healthEventType =
        static_cast<HealthEventType>(selData.offset);

    switch (healthEventType)
    {
        case HealthEventType::FirmwareStatus:
            return fw_status::messageHook(selData, eventId, parameters);
            break;

        case HealthEventType::SmbusLinkFailure:
            return smbus_failure::messageHook(selData, eventId, parameters);
            break;
    }
    return false;
}
}
```
##### 2. Call designated handler
Example of handler for `FirmwareStatus`, tailored down to essential distinctive
use cases:
```cpp
namespace fw_status {
static bool messageHook(const SELData& selData, std::string& eventId,
                        std::vector<std::string>& parameters)
{
    // Maps EventData[0] to either a resolution or further action
    static const boost::container::flat_map<
        uint8_t,
        std::pair<std::string, std::optional<std::variant<utils::ParserFunc,
                                                          utils::MessageMap>>>>
        eventMap = {
            // EventData[0]=0
            // > MessageId=MERecoveryGpioForced
            {0x00, {"MERecoveryGpioForced", {}}},

            // EventData[0]=3
            // > call specific handler do determine MessageId and Parameters
            {0x03, {{}, flash_state::messageHook}},

            // EventData[0]=7
            // > MessageId=MEManufacturingError
            // > Use manufacturingError map to translate EventData[1] to string
            //   and add it to Parameters collection
            {0x07, {"MEManufacturingError", manufacturingError}},

            // EventData[0]=9
            // > MessageId=MEFirmwareException
            // > Use a function to log specified byte of payload as Parameter
            //   in chosen format. Here it stores 2-nd byte in hex format.
            {0x09, {"MEFirmwareException", utils::logByteHex<2>}}

    return utils::genericMessageHook(eventMap, selData, eventId, parameters);
}

// Maps EventData[1] to specified message
static const boost::container::flat_map<uint8_t, std::string>
    manufacturingError = {
        {0x00, "Generic error"},
        {0x01, "Wrong or missing VSCC table"}}};
}
```

##### 3. Store parsed log in system
Cascading calls of functions, logging utilities and map resolutions are
resulting in populating both `std::string& eventId` and
`std::vector<std::string>& parameters`. This data is then used to form a valid
system log and stored in system journal.

#### Event data listing
Event data is accessible as `Redfish` resources in two places:
- `MessageRegistry` - stores all event 'metadata'
    (severity, resolution notes, messageId)
- `EventLog` - lists all detected events in the system in processed,
    human-readable form

##### MessageRegistry
Implementation of `bmcweb` [MessageRegistry](http://redfish.dmtf.org/schemas/v1/MessageRegistry.json)
contents can be found at  `openbmc/bmcweb/redfish-core/include/registries/openbmc_message_registry.hpp`.

**Intel-specific events have proper prefix in MessageId: either 'BIOS' or 'ME'.**

It can be read by the user by calling `GET` on Redfish resource:
`/redfish/v1/Registries/OpenBMC/OpenBMC`. It contains JSON array of entries
in standard Redfish format, like so:
```json
"MEFlashWearOutWarning": {
    "Description": "Indicates that Intel ME has reached certain threshold of flash write operations.",
    "Message": "Warning threshold for number of flash operations has been exceeded. Current percentage of write operations capacity: %1",
    "NumberOfArgs": 1,
    "ParamTypes": [
        "number"
    ],
    "Resolution": "No immediate repair action needed.",
    "Severity": "Warning"
}
```

##### EventLog
System-wide [EventLog](http://redfish.dmtf.org/schemas/v1/LogService.json)
is implemented in `bmcweb` at  `openbmc/bmcweb/redfish-core/lib/log_services.hpp`.

It can be read by the user by calling `GET` on Redfish resource:
`/redfish/v1/Systems/system/LogServices/EventLog`. It contains JSON array
of log entries in standard Redfish format, like so:
```json
{
    "@odata.id": "/redfish/v1/Systems/system/LogServices/EventLog/Entries/37331",
    "@odata.type": "#LogEntry.v1_4_0.LogEntry",
    "Created": "1970-01-01T10:22:11+00:00",
    "EntryType": "Event",
    "Id": "37331",
    "Message": "Warning threshold for number of flash operations has been exceeded. Current percentage of write operations capacity: 50",
    "MessageArgs": [
        "50"
    ],
    "MessageId": "OpenBMC.0.1.MEFlashWearOutWarning",
    "Name": "System Event Log Entry",
    "Severity": "Warning"
}
```

## References
1. [IPMI Specification v2.0](https://www.intel.pl/content/www/pl/pl/products/docs/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html)
2. [DMTF Redfish Schema Guide](https://www.dmtf.org/sites/default/files/standards/documents/DSP2046_2019.3.pdf)
3. [OpenBMC](https://github.com/openbmc)
4. [OpenBMC Redfish Event logging](https://github.com/openbmc/docs/blob/master/architecture/redfish-logging-in-bmcweb.md)
5. [Intel ME External Interfaces Specification](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/intel-power-node-manager-v3-spec.pdf)
6. [ipmitool](https://github.com/ipmitool/ipmitool)
7. [OpenBMC Intel IPMI support](https://github.com/openbmc/intel-ipmi-oem)
8. [OpenBMC BMCWeb](https://github.com/openbmc/bmcweb)
