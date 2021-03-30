Copyright 2018 IBM

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

# Introduction

This document defines a protocol and several transports for flash access
mediation between the host and the Baseboard Management Controller (BMC).

The driving motivation for the protocol is to expose flash devices owned by the
BMC to the host. Usually, the flash device of interest is the host's firmware
flash device - in some platform designs this is owned by the BMC to enable
lights-out updates of the host firmware.

As the flash is owned by the BMC, access by the host to its firmware must be
abstracted and mediated. Abstraction and mediation alleviates several problems:

1. Proliferation of flash controller driver implementations throughout firmware
2. Conflict of access between the host and the BMC to the flash controller
3. In some circumstances, mitigates security concerns.

The protocol introduced in this document addresses each of these issues.
Specifically, the document addresses defining a control mechanism for exposing
flash data in the LPC firmware space, communicated via functions in the LPC IO
space.

# Scope

The scope of the document is limited to defining the protocol and its
transports, and does not cover the content or structure of the data read or
written to the flash by the host firmware.

The definition of transport-specific parameters, for example selection of IPMI
`(NetFn, Command)` pairs, is also beyond the scope of the document.

# Background, Design and Constraints

The protocol was developed to meet requirements on OpenPOWER systems based
around the ASPEED BMC System-on-Chips such as the AST2400 and AST2500.
The ASPEED BMCs have properties and features that need to be taken into account:

1. A read-only mapping of the SPI flash devices onto the ARM core's AHB
2. Remapping of LPC Firmware cycles onto the AHB (LPC2AHB bridge)
3. A reasonable but not significant amount of attached DRAM

Prior to the development of the protocol described below flash reads were
serviced by pointing the LPC2AHB bridge at the read-only flash mapping, and
writes were serviced by a separate bridge that suffers significant performance
and security issues.

Point 3 serves to justify some of the design decisions embodied in the
protocol, mainly the complexity required by the flexibility to absorb as much
or as little reserved memory as desired via the concept of BMC-controlled
windowing.

The core concept of the protocol moves access away from the naive routing of
LPC firmware cycles onto the host firmware SPI flash AHB mapping, and
concentrates on servicing the LPC firmware cycles from reserved system memory.
As the memory backing the LPC2AHB mapping is now writable the protocol meets
the host's write requirements by defining commands to open, dirty and flush an
in-memory window representing the state of the flash. The mechanism to read the
flash becomes same as write, just that the dirty and flush commands are not
legal on such windows.

## Historic and Future Naming

The original transport for the protocol was the ASPEED BMC LPC mailbox
interface, and previous revisions of the protocol documentation referred to the
protocol as the "mailbox" or "mbox" protocol. This naming is now deprecated, as
the protocol has grown further transports and naming the protocol by its
transport rather than its intent was, on reflection, misguided.

The protocol has been tentatively renamed to the "Host I/O Mapping Protocol" or
"hiomap". This is a reflection of its true purpose - to control the host's view
of data exposed from the BMC.

# Protocol Overview

The primary flow of the protocol is for the host to send requests to the BMC,
which adjusts the mapping of the LPC firmware space as requested and returns a
status response to the host. These interactions are labelled "commands".
However, as there is now an active software component on the BMC consuming
access requests, the BMC must occasionally indicate state changes to the host.
Such interactions are labelled "events". For example, if a user or other system
software on the BMC suspends the host's access to its flash device, the
BMC-side daemon implementing the protocol must notify the host that its
requests will be denied until further notice.

## Protocol Versioning

To enable evolution of the command and event interfaces, incremental changes to
the behaviour are defined in new versions of the protocol. The descriptions
and tables that follow all identify the versions to which they are applicable.

The highest currently specified protocol version is version 3.

## Table of Commands

| ID | Name | v1 | v2 | v3 | Description |
|----|------|----|----|----|-------------|
|  1 | [`RESET`](#reset-command) | ✓ | ✓ | ✓ | Reset the state of the LPC firmware space, closing any active window |
|  2 | [`GET_INFO`](#get_info-command) | ✓ | ✓ | ✓ | Perform protocol version negotiation and retrieve fundamental parameters |
|  3 | [`GET_FLASH_INFO`](#get_flash_info-command) | ✓ | ✓ | ✓ | Retrieve flash-specific parameters |
|  4 | [`CREATE_READ_WINDOW`](#create_read_window-command) | ✓ | ✓ | ✓ | Request mapping of a flash region for read |
|  5 | [`CLOSE`](#close-command) | ✓ | ✓ | ✓ | Close the current window, flushing any dirty regions |
|  6 | [`CREATE_WRITE_WINDOW`](#create_write_window-command) | ✓ | ✓ | ✓ | Request mapping of a flash region for write |
|  7 | [`MARK_DIRTY`](#mark_dirty-command) | ✓ | ✓ | ✓ | Mark a region of a write window as modified |
|  8 | [`FLUSH`](#flush-command) | ✓ | ✓ | ✓ | Flush dirty regions of the write window to flash |
|  9 | [`ACK`](#ack-command) | ✓ | ✓ | ✓ | Acknowledge the receipt of an event from the BMC |
| 10 | [`ERASE`](#erase-command) |   | ✓ | ✓ | Mark a region of a write window as erased |
| 11 | [`GET_FLASH_NAME`](#get_flash_name-command) |   |   | ✓ | Retrieve the name of an indexed flash device |
| 12 | [`LOCK`](#lock-command) |   |   | ✓ | Mark a region of the current flash window as immutable |

## Table of Events

| ID | Name | v1 | v2 | v3 | Description |
|----|------|----|----|----|-------------|
|  0 | [`PROTOCOL_RESET`](#protocol_reset-event) | ✓ | ✓ | ✓ | The host is required to perform version negotiation and re-establish its window of interest |
|  1 | [`WINDOW_RESET`](#window_reset-event) |   | ✓ | ✓ | The host must re-establish its window of interest |
|  6 | [`FLASH_CONTROL_LOST`](#flash_control_lost-event) |   | ✓ | ✓ | The host should suspend access requests |
|  7 | [`DAEMON_READY`](#daemon_ready-event) |   | ✓ | ✓ | The daemon is active and can accept commands |

## List of Transports

An essential feature of the protocol is that its behaviour is independent of
the host-BMC transport. The command and event interfaces of each transport
necessarily reflect the version of the protocol they are implementing, but the
transport has no influence otherwise.

There are three documented transports for the protocol:

1. The [ASPEED BMC LPC Mailbox transport](#mailbox-transport)
2. The [IPMI transport](#ipmi-transport)
3. The [DBus transport](#dbus-transport)

The command layout, routing and event mechanism for each transport all have
different features and are detailed below. An important note is that command
design is limited by the most constrained transport - the LPC mailbox
transport - where only 11 bytes are available for encoding of command
parameters.

Implementations must choose to support one or more of the transports outlined
in this document.

Note that the DBus transport is intended for BMC-internal communications, and
can be used to separate a host-interface transport from the protocol
implementation.

## Protocol Flow

The high-level protocol flow is that the host first issues a `GET_INFO` command
to negotiate the protocol version and acquire parameters fundamental to
constructing arguments to and interpreting responses from the commands that
follow.

Once `GET_INFO` has successfully completed, the host should request the flash
parameters with `GET_FLASH_INFO`. The response provides information on the
flash capacity and the size of its erase granule.

Following `GET_FLASH_INFO`, the next act is to establish an active flash window
with either one of the `CREATE_READ_WINDOW` or `CREATE_WRITE_WINDOW` commands.

In the event of creating a write window the host must inform the BMC of the
regions to which it has written with `MARK_DIRTY`- the BMC receives no
notification of accesses from the host, they are simply mapped by the LPC2AHB
bridge as necessary. As the accesses are to system memory and not the flash
the changes identified by the `MARK_DIRTY` commands are not permanent until a
`FLUSH` command is received (implicit flushes are discussed below), at which
point the dirty regions of the active window will be written to the flash
device.

As an optimisation the host may choose to use the `ERASE` command to indicate
that large regions should be set to the erased state. This optimisation saves
the associated LPC firmware cycles to write the regions into the erased state.

## Version Negotiation

When invoking `GET_INFO` the host must provide the BMC its highest
supported version of the protocol. The BMC must respond with a protocol version
less than or equal to that requested by the host, or in the event that there is
no such value, an error code. In the event that an error is returned the host
must not continue to communicate with the BMC. Otherwise, the protocol version
returned by the BMC is the agreed protocol version for all further
communication. The host may at a future point request a change in protocol
version by issuing a subsequent `GET_INFO` command.

### Unversioned Commands

In some circumstances it is necessary for bootstrap or optimisation purposes to
support unversioned commands. The protocol supports three unversioned commands:

1. `RESET`
2. `GET_INFO`
3. `ACK`

All remaining commands have their presence and behaviour specified with respect
to the negotiated version of the protocol.

The arguments to the `GET_INFO` command are considered unversioned and as a
result are static in nature - the protocol implementation has no means to
decode version-specific arguments as the version has not yet been
negotiated. With respect to the response, the version field is unversioned, but
all subsequent fields _may_ be versioned.

`RESET` remaining unversioned is an optimisation catering to deeply embedded
components on the host side that may need access to the command. Keeping
`RESET` unversioned removes the complexity of implementing `GET_INFO` with its
version negotiation and minimises the overhead required to get into the
pre-boot state.

Defining `ACK` as unversioned ensures host firmware that has minimal protocol
support can silence interrupts from the BMC as required.

## Sequence Numbers

Sequence numbers are included in messages for correlation of commands and
responses. v1, v2 and v3 of the protocol permit either zero or one commands to
be in progress (yet to receive a response).

For generality, the host must generate a sequence number that is unique with
respect to the previous command (one that has received a response) and any
in-progress commands. Sequence numbers meeting this requirement are considered
valid. The BMC's response to a command must contain the same sequence number
issued by the host as found in the relevant command.

Sequence numbers may be reused in accordance with the constraints outlined
above. However, it is not an error if the BMC receives a unversioned command
(`RESET`, `GET_INFO` or `ACK`) with an invalid sequence number. For all other
cases, the BMC must respond with an error if the constraints are violated. If
the host receives a sequence-related error response it must consider any
in-progress commands to have failed. The host may retry the affected command(s)
after generating a suitable sequence number.

## Window Management

There is only ever one active window which is the window created by the most
recent `CREATE_READ_WINDOW` or `CREATE_WRITE_WINDOW` call which succeeded. Even
though there are two types of windows there can still only be one active window
irrespective of type. The host must not write to a read window. The host may
read from a write window and the BMC must guarantee that the window reflects
what the host has written there.

A window can be closed by issuing the `CLOSE` command, in which case there is
no active window and the host must not access the LPC firmware space until a
window is subsequently opened. If the host closes an active write window then
the BMC must perform an implicit flush. If the host tries to open a new window
with an already active window then the active window is closed (and implicitly
flushed if it was a write window). If the new window is successfully opened
then it is the new active window; if the command fails then there is no active
window and the previously active window must no longer be accessed.

The host must not access an LPC address other than that which is contained by
the active window. The host must not use write management functions (see below)
if the active window is a read window or if there is no active window.

## Command Parameter Types

It is common in the protocol definition for command parameters to be
represented in terms of a block size. This block size may refer to e.g. the
size of the erase granule of the flash, or it may be another value entirely.
Regardless of what it represents, the argument values are scaled by the block
size determined by version negotiation. Specifying arguments in terms of a
block size allows transports to keep a compact representation in constrained
implementations such as the LPC mailbox transport.

Note that for simplicity block size must always be a power-of-2. The block size
must also be greater than or equal to 4K regardless of the negotiated protocol
version.

Finally, conversion between blocks and bytes is achieved by respectively
dividing or multiplying the quantity by the negotiated block-size.

# Transport Overview

Several transports are defined for the protocol and are outlined below. The key
features of transport support are the wire-format, delivery mechanisms of
commands and events, and the definition and delivery of response codes.

The DBus transport is the most foreign of the three as it does not encode the
command index or a sequence number; these two elements are handled by the
properties of DBus itself.

## Mailbox Transport

* Multi-byte quantity endianness: Little-endian
* Command length encoding: Assumed from negotiated protocol version
* Parameter alignment: Packed (no padding)
* Command status response: ABI-defined Response byte
* Event Delivery: ABI-defined BMC Status byte

The mailbox transport defines the ABI used over the mailbox registers. There
are 16 data registers and several status and control registers for managing
interrupts between the host and the BMC. For the purpose of defining the
transport ABI the status and control registers can mostly be disregarded, save
for the necessity of issuing and responding to interrupts on each side.

Assuming the registers are in a contiguous layout (this is not reflected in the
hardware, but may be the abstraction presented by the associated kernel
driver), the ABI is defined as follows, where the bytes in the range [2, 12]
are available for command parameters and are defined on a per-command basis.

```
    0        7         15                    31
   +----------+----------+---------------------+
 0 | Command  | Sequence |                     |
   +----------+----------+---------------------+
 4 |                                           |
   +-------------------------------------------+
 8 |                                           |
   +----------+----------+----------+----------+
12 |          | Response |  BMC Sts | Host Sts |
   +----------+----------+----------+----------+
    0        7         15         23         31
```

Command status response codes are as follows:

### Status Codes

| ID |     Name       | v1 | v2 | v3 | Description                                          |
|----|----------------|----|----|----|------------------------------------------------------|
|  1 | `SUCCESS`      | ✓  | ✓  | ✓  | Command completed successfully                       |
|  2 | `PARAM_ERROR`  | ✓  | ✓  | ✓  | Error with parameters supplied or command invalid    |
|  3 | `WRITE_ERROR`  | ✓  | ✓  | ✓  | Error writing to the backing file system             |
|  4 | `SYSTEM_ERROR` | ✓  | ✓  | ✓  | Error in BMC performing system action                |
|  5 | `TIMEOUT`      | ✓  | ✓  | ✓  | Timeout in performing action                         |
|  6 | `BUSY`         |    | ✓  | ✓  | Flash access suspended, retry later                  |
|  7 | `WINDOW_ERROR` |    | ✓  | ✓  | Invalid window state or command invalid for window   |
|  8 | `SEQ_ERROR`    |    | ✓  | ✓  | Invalid sequence number supplied with command        |
|  9 | `LOCKED_ERROR` |    |    | ✓  | Erased or dirtied region intersected a locked region |

## IPMI Transport

* Multi-byte quantity endianness: Little-endian
* Command length encoding: Assumed from negotiated protocol version
* Parameter alignment: Packed (no padding)
* Command status response: Mapped to IPMI completion codes
* Event Delivery: Status byte in SEL via `SMS_ATN`

The IPMI transport must reserve one `(NetFn, Command)` pair for host-to-BMC
communications and one SEL `(NetFn, Command)` pair for BMC-to-host
communication, signaled by `SMS_ATN`.

The wire command framing is as follows:

1. The command identifier is the first value and is encoded in one byte
2. The sequence number is the second value and is encoded in one byte
3. Parameters required by the (version, command) pair follow

```
    0        7         15                     N
   +----------+----------+---------     -------+
 0 | Command  | Sequence |          ...        |
   +----------+----------+---------     -------+
```

## DBus Transport

* Multi-byte quantity endianness: Transport encoded
* Command length encoding: Transport encoded
* Parameter alignment: Transport encoded
* Command status response: Mapped to Unix system error codes
* Event Delivery: DBus signals and properties per event type

DBus defines its own wire-format for messages, and so support for this
transport concentrates on mapping commands and events onto DBus concepts.
Specifically, commands are represented by DBus methods and events are
represented by properties. DBus will automatically generate a
`PropertiesChanged` signal for changes to properties allowing consumers to
learn of updates as they happen.

As the commands are represented by DBus methods there is no need to encode the
command index in the request - this is represented by the appropriate method on
the implementation object's interface.

Similarly, there's no need to encode sequence numbers as DBus handles the
correlation of messages over the bus. As there is no encoding of sequence
numbers, there is no need to describe a command status response like
`SEQ_ERROR`, which allows a clean mapping to Unix error codes.

Finally, commands mapped to methods have the number of parameters and types
described by the method's type signature, though these descriptions concern the
basic wire types and not the semantic types relevant to the protocol. The method
type signature and parameter ordering are described in the relevant command
definition.

# Command Definitions

The command identifier values and command-response parameter formats are
described in tables under headers for each command. The order of the parameters
in the parameter tables reflects the order of the parameters in the commands
and responses. The M, I, and D columns represent the Mailbox, IPMI and DBus
transports respectively. For the command identifier table the value in these
columns' cells represent the command index, or for DBus, its method name. For
the parameter tables the value represent the parameter's offset in the message
(disregarding the command and sequence bytes), or in the case of DBus the
appropriate [type
signature](https://dbus.freedesktop.org/doc/dbus-specification.html#basic-types).

## `RESET` Command

| v1 | v2 | v3 | M | I | D     |
|----|----|----|---|---|-------|
| ✓  | ✓  | ✓  | 1 | 1 | Reset |

### v1 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>


### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### Description

Requests the BMC return the LPC firmware space to a state ready for host
firmware bootstrap.

## `GET_INFO` Command

| v1 | v2 | v3 | M | I | D       |
|----|----|----|---|---|---------|
| ✓  | ✓  | ✓  | 2 | 2 | GetInfo |

### v1 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Version           | Version |   1  | 0 | 0 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Version           | Version |   1  | 0 | 0 | y |
| Read Window Size  | Blocks  |   2  | 1 | 1 | q |
| Write Window Size | Blocks  |   2  | 3 | 3 | q |

</td>
</tr>
</table>


### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Version           | Version |   1  | 0 | 0 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Version           | Version |   1  | 0 | 0 | y |
| Block Size Shift  | Count   |   1  | 5 | 1 | y |
| Timeout           | Seconds |   2  | 6 | 2 | q |

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Version           | Version |   1  | 0 | 0 | y |
| Block Size Shift  | Count   |   1  | 1 | 1 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Version           | Version |   1  | 0 | 0 | y |
| Block Size Shift  | Count   |   1  | 5 | 1 | y |
| Timeout           | Seconds |   2  | 6 | 2 | q |
| Devices           | Count   |   1  | 8 | 4 | y |

</td>
</tr>
</table>

### Description

The suggested timeout is a hint to the host as to how long it should wait after
issuing a command to the BMC before it times out waiting for a response. This
is the maximum time which the BMC thinks it could take to service any command
which the host could issue. This may be set to zero to indicate that the BMC
does not wish to provide a hint in which case the host must choose some
reasonable value.

From v3 the host may desire a specific block size and thus can request this by
giving a hint to the daemon (may be zero). The daemon may use this to select
the block size which it will use however is free to ignore it. The value in the
response is the block size which must be used for all further requests until a
new size is negotiated by another call to `GET_INFO`.

## `GET_FLASH_INFO` Command

| v1 | v2 | v3 | M | I | D            |
|----|----|----|---|---|--------------|
| ✓  | ✓  | ✓  | 3 | 3 | GetFlashInfo |

### v1 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Size        | Bytes   | 4    | 0 | 0 | u |
| Erase Granule     | Bytes   | 4    | 4 | 4 | u |

</td>
</tr>
</table>


### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Size        | Blocks  | 2    | 0 | 0 | q |
| Erase Granule     | Blocks  | 2    | 2 | 2 | q |

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Device ID         | Index   | 1    | 0 | 0 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Size        | Blocks  | 2    | 0 | 0 | q |
| Erase Granule     | Blocks  | 2    | 2 | 2 | q |

</td>
</tr>
</table>

## `CREATE_READ_WINDOW` Command

| v1 | v2 | v3 | M | I | D                |
|----|----|----|---|---|------------------|
| ✓  | ✓  | ✓  | 4 | 4 | CreateReadWindow |

### v1 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Offset      | Blocks  | 2    | 0 | 0 | q |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| LPC FW Offset     | Blocks  | 2    | 0 | 0 | q |

</td>
</tr>
</table>


### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Address     | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| LPC FW Address    | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |
| Flash Address     | blocks  | 2    | 4 | 4 | q |

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Address     | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |
| Device ID         | Index   | 1    | 4 | 4 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| LPC FW Address    | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |
| Flash Address     | blocks  | 2    | 4 | 4 | q |

</td>
</tr>
</table>

### Description

The flash offset which the host requests access to is always taken from the
start of flash - that is it is an absolute offset into flash.

LPC bus address is always given from the start of the LPC address space - that
is it is an absolute address.

The requested access size is only a hint. The response indicates the actual
size of the window. The BMC may want to use the requested size to pre-load the
remainder of the request. The host must not access past the end of the active
window.

The flash offset mapped by the window is an absolute flash offset and must be
less than or equal to the flash offset requested by the host. It is the
responsibility of the host to use this information to access any offset which
is required.

The requested window size may be zero. In this case the BMC is free to create
any sized window but it must contain at least the first block of data requested
by the host. A large window is of course preferred and should correspond to the
default size returned in the `GET_INFO` command.

If this command returns successfully then the created window is the active
window. If it fails then there is no active window.

## `CLOSE` Command

| v1 | v2 | v3 | M | I | D     |
|----|----|----|---|---|-------|
| ✓  | ✓  | ✓  | 5 | 5 | Close |

### v1 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>


### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flags             | Field   | 1    | 0 | 0 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flags             | Field   | 1    | 0 | 0 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### Description

Closes the active window. Any further access to the LPC bus address specified
to address the previously active window will have undefined effects. If the
active window is a write window then the BMC must perform an implicit flush.

The Flags argument allows the host to provide some hints to the BMC. Defined
values are:

```
0x01 - Short Lifetime:
       The window is unlikely to be accessed anytime again in the near future.
       The effect of this will depend on BMC implementation. In the event that
       the BMC performs some caching the BMC daemon could mark data contained
       in a window closed with this flag as first to be evicted from the cache.
```

## `CREATE_WRITE_WINDOW` Command

| v1 | v2 | v3 | M | I | D                 |
|----|----|----|---|---|-------------------|
| ✓  | ✓  | ✓  | 6 | 6 | CreateWriteWindow |

### v1 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Offset      | Blocks  | 2    | 0 | 0 | q |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| LPC FW Offset     | Blocks  | 2    | 0 | 0 | q |

</td>
</tr>
</table>


### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Address     | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| LPC FW Address    | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |
| Flash Address     | blocks  | 2    | 4 | 4 | q |

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Address     | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |
| Device ID         | Index   | 1    | 4 | 4 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| LPC FW Address    | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |
| Flash Address     | blocks  | 2    | 4 | 4 | q |

</td>
</tr>
</table>

### Description

The flash offset which the host requests access to is always taken from the
start of flash - that is it is an absolute offset into flash.

LPC bus address is always given from the start of the LPC address space - that
is it is an absolute address.

The requested access size is only a hint. The response indicates the actual
size of the window. The BMC may want to use the requested size to pre-load the
remainder of the request. The host must not access past the end of the active
window.

The flash offset mapped by the window is an absolute flash offset and must be
less than or equal to the flash offset requested by the host. It is the
responsibility of the host to use this information to access any offset which
is required.

The requested window size may be zero. In this case the BMC is free to create
any sized window but it must contain at least the first block of data requested
by the host. A large window is of course preferred and should correspond to the
default size returned in the `GET_INFO` command.

If this command returns successfully then the created window is the active
window. If it fails then there is no active window.

## `MARK_DIRTY` Command

| v1 | v2 | v3 | M | I | D         |
|----|----|----|---|---|-----------|
| ✓  | ✓  | ✓  | 7 | 7 | MarkDirty |

### v1 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Offset      | Blocks  | 2    | 0 | 0 | q |
| Length            | Bytes   | 4    | 2 | 2 | u |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>


### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Window Offset     | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Window Offset     | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |
| Flags             | Field   | 1    | 4 | 4 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### Description


The BMC has no method for intercepting writes that
occur over the LPC bus. The host must explicitly notify
the daemon of where and when a write has occurred so it
can be flushed to backing storage.

Offsets are given as an absolute (either into flash (V1) or the
active window (V2)) and a zero offset refers to the first
block. If the offset + number exceeds the size of the active
window then the command must not succeed.

The host can give a hint to the daemon that is doesn't have to erase a flash
area before writing to it by setting bit zero of the Flags parameter. This
means that the daemon will blindly perform a write to that area and will not
try to erase it before hand.  This can be used if the host knows that a large
area has already been erased for example but then wants to perform many small
writes.

## `FLUSH` Command

| v1 | v2 | v3 | M | I | D     |
|----|----|----|---|---|-------|
| ✓  | ✓  | ✓  | 8 | 8 | Flush |

### v1 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Offset      | Blocks  | 2    | 0 | 0 | q |
| Length            | Bytes   | 4    | 2 | 2 | u |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>


### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### Description

Flushes any dirty/erased blocks in the active window to the backing storage.

In V1 this can also be used to mark parts of the flash dirty and flush in a
single command. In V2 the explicit mark dirty command must be used before a
call to flush since there are no longer any arguments. If the offset + length
exceeds the size of the active window then the command must not succeed.

## `ACK` Command

| v1 | v2 | v3 | M | I | D   |
|----|----|----|---|---|-----|
| ✓  | ✓  | ✓  | 9 | 9 | Ack |

### v1 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Ack Mask          | Field   | 1    | 0 | 0 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>


### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Ack Mask          | Field   | 1    | 0 | 0 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Ack Mask          | Field   | 1    | 0 | 0 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### Description

The host should use this command to acknowledge BMC events propagated to the
host.

## `ERASE` Command

| v1 | v2 | v3 | M  | I  | D     |
|----|----|----|----|----|-------|
|    | ✓  | ✓  | 10 | 10 | Erase |

### v2 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Window Offset     | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Window Offset     | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### Description

This command allows the host to erase a large area without the need to
individually write 0xFF repetitively.

Offset is the offset within the active window to start erasing from (zero
refers to the first block of the active window) and number is the number of
blocks of the active window to erase starting at offset. If the offset + number
exceeds the size of the active window then the command must not succeed.

## `GET_FLASH_NAME` Command

| v1 | v2 | v3 | M  | I  | D            |
|----|----|----|----|----|--------------|
|    |    | ✓  | 11 | 11 | GetFlashName |


### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Device ID         | Index   | 1    | 0 | 0 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Name length       | Bytes   | 1    | 0 | 0 | - |
| Name              | String  | 10   | 1 | 1 | s |

</td>
</tr>
</table>

### Description

Describes a flash with some kind of identifier useful to the host system.

The length in the response is the number of response arguments as part of the
flash name field which the host should expect to have been populated.

Note that DBus encodes the string length in its string type, so the explicit
length is omitted from the DBus message.

## `LOCK` Command

| v1 | v2 | v3 | M  | I  | D    |
|----|----|----|----|----|------|
|    |    | ✓  | 12 | 12 | Lock |

### v3 Parameters

<table>
<tr>
<th>Command</th><th>Response</th>
</tr>
<tr>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|
| Flash Offset      | Blocks  | 2    | 0 | 0 | q |
| Length            | Blocks  | 2    | 2 | 2 | q |
| Device ID         | Index   | 1    | 4 | 4 | y |

</td>
<td valign="top">

| Parameter         | Unit    | Size | M | I | D |
|-------------------|---------|------|---|---|---|

</td>
</tr>
</table>

### Description

Lock an area of flash so that the host can't mark it dirty or erased. If the
requested area is within the current window and that area is currently marked
dirty or erased then this command must fail.

# Event Definitions

The M, I, and D columns represent the Mailbox, IPMI and DBus transports
respectively. The values in the M, I or D columns represent the events' bit
index in the status byte, or in the case of the DBus transport the name of the
relevant property.

For the DBus interface, properties are used for all event types regardless of
whether they should be acknowledged or not as part of the protocol. This
ensures that state changes can be published atomically.

## `PROTOCOL_RESET` Event

| v1 | v2 | v3 | M | I | D             |
|----|----|----|---|---|---------------|
| ✓  | ✓  | ✓  | 0 | 0 | ProtocolReset |

### Description

Used to inform the host that a protocol reset has occurred, likely due to
restarting the daemon. The host must perform protocol version negotiation again
and must assume it has no active window. The host must also assume that any
in-flight commands have failed.

## `WINDOW_RESET` Event

| v1 | v2 | v3 | M | I | D           |
|----|----|----|---|---|-------------|
|    | ✓  | ✓  | 1 | 1 | WindowReset |

### Description

The host must assume that its active window has been closed and that it no
longer has an active window. The host is not required to perform protocol
version negotiation again. The host must assume that any in-flight commands
have failed.

## `FLASH_CONTROL_LOST` Event

| v1 | v2 | v3 | M | I | D                |
|----|----|----|---|---|------------------|
|    | ✓  | ✓  | 6 | 6 | FlashControlLost |

### Description

The BMC daemon has been suspended and thus no longer controls access to the
flash (most likely because some other process on the BMC required direct access
to the flash and has suspended the BMC daemon to preclude concurrent access).

The BMC daemon must clear this bit itself when it regains control of the flash
(the host isn't able to clear it through an acknowledge command).

The host must not assume that the contents of the active window correctly
reflect the contents of flash while this bit is set.

## `DAEMON_READY` Event

| v1 | v2 | v3 | M | I | D           |
|----|----|----|---|---|-------------|
|    | ✓  | ✓  | 7 | 7 | DaemonReady |

### Description

Used to inform the host that the BMC daemon is ready to accept command
requests. The host isn't able to clear this bit through an acknowledge command,
the BMC daemon must clear it before it terminates (assuming it didn't terminate
unexpectedly).

The host should not expect a response while this bit is not set.

Note that this bit being set is not a guarantee that the BMC daemon will
respond as it or the BMC may have crashed without clearing it.
