Copyright 2017,2018 IBM

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## Intro

This document describes a protocol for host to BMC communication via the
mailbox registers present on the Aspeed 2400 and 2500 chips.
This protocol is specifically designed to allow a host to request and manage
access to a flash device(s) with the specifics of how the host is required to
control this described below.

## Version

Version specific protocol functionalities are represented by the version number
in brackets next to the definition of the functionality. (e.g. (V2) for version
2 specific funtionality). All version specific functionality must also be
implemented by proceeding versions up to and not including the version a command
was removed.

## Problem Overview

"mbox" is the name we use to represent a protocol we have established between
the host and the BMC via the Aspeed mailbox registers. This protocol is used
for the host to control access to the flash device(s).

Prior to the mbox protocol, the host uses a backdoor into the BMC address space
(the iLPC-to-AHB bridge) to directly manipulate the BMCs own flash controller.

This is not sustainable for a number of reasons. The main ones are:

1. Every piece of the host software stack that needs flash access (HostBoot,
   OCC, OPAL, ...) has to have a complete driver for the flash controller,
   update it on each BMC generation, have all the quirks for all the flash
   chips supported etc... We have 3 copies on the host already in addition to
   the one in the BMC itself.

2. There are serious issues of access conflicts to that controller between the
   host and the BMC.

3. It's very hard to support "BMC reboots" when doing that

4. It's slow

5. Last but probably most important, having that backdoor open is a security
   risk. It means the host can access any address on the BMC internal bus and
   implant malware in the BMC itself. So if the host is a "bare metal" shared
   system in some kind of data center, not only the host flash needs to be
   reflashed when switching from one customer to another, but the entire BMC
   flash too as nothing can be trusted. So we want to disable it.

To address all these, we have implemented a new mechanism that we call mbox.

When using this mechanism, the BMC is solely responsible for directly accessing
the flash controller. All flash erase and write operations are performed by the
BMC and the BMC only. (We can allow direct reads from flash under some
circumstances but we tend to prefer going via memory).

The host uses the mailbox registers to send "commands" to the BMC, which
responds via the same mechanism. Those commands allow the host to control a
"window" (which is the LPC -> AHB FW space mapping) that is either a read
window or a write window onto the flash.

When set for writing, the BMC makes the window point to a chunk of RAM instead.
When the host "commits" a change (via MBOX), then the BMC can perform the
actual flashing from the data in the RAM window.

The idea is to have the LPC FW space be routed to an active "window".  That
window can be a read or a write window. The commands allow to control which
window and which offset into the flash it maps.

* A read window can be a direct window to the flash controller space (ie.
  0x3000\_0000) or it can be a window to a RAM image of a flash. It doesn't have
  to be the full size of the flash per protocol (commands can be used to "slide"
  it to various parts of the flash) but if it's set to map the actual flash
  controller space at 0x3000\_0000, it's probably simpler to make it the full
  flash. The host makes no assumption, it's your choice what to provide. The
  simplest implementation is to just route to the flash read/only.

* A write window has to be a chunk of BMC memory. The minimum size is not
  defined in the spec, but it should be at least one block (4k for now but it
  should support larger block sizes in the future). When the BMC receive the
  command to map the write window at a given offset of the flash, the BMC should
  copy that portion of the flash into a reserved memory buffer, and modify the
  LPC mapping to point to that buffer.

The host can then write to that window directly (updating the BMC memory) and
send a command to "commit" those updates to flash.

Finally, there is a `RESET_STATE`. It's the state in which the bootloader in the
SEEPROM of the POWER9 chip will find what it needs to load HostBoot. The
details are still being ironed out: either mapping the full flash read only or
reset to a "window" that is either at the bottom or top of the flash. The
current implementation resets to point to the full flash.

## Where is the code?

The mbox userspace is available [on GitHub](https://github.com/openbmc/mboxbridge)
This is Apache licensed but we are keen to see any enhancements you may have.

The kernel driver is still in the process of being upstreamed but can be found
in the OpenBMC Linux kernel staging tree:

https://github.com/openbmc/linux/commit/85770a7d1caa6a1fa1a291c33dfe46e05755a2ef

## Building

The Autotools of this requires the autoconf-archive package for your
system

## The Hardware

The Aspeed mailbox consists of 16 (8 bit) data registers see Layout for their
use. Mailbox interrupt enabling, masking and triggering is done using a pair
of control registers, one accessible by the host the other by the BMC.
Interrupts can also be raised per write to each data register, for BMC and
host. Write triggered interrupts are configured using two 8 bit registers where
each bit represents a data register and if an interrupt should fire on write.
Two 8 bit registers are present to act as a mask for write triggered
interrupts.

### Layout

```
Byte 0: COMMAND
Byte 1: Sequence
Byte 2-12: Arguments
Byte 13: Response code
Byte 14: Host controlled status reg
Byte 15: BMC controlled status reg
```

Note: when the BMC is writing a response to the mbox registers (as described
above), the "Response Code" (Register 13) must be the last register written to.
Writing register 13 will trigger an interrupt to the host indicating a complete
response has been written. Triggering the interrupt by writing register 13
prior to completing the response may lead to a data race, and must, therefore,
be avoided.

## Low Level Protocol Flow

What we essentially have is a set of registers which either the host or BMC can
write to in order to communicate to the other which will respond in some way.
There are 3 basic types of communication.

1. Commands sent from the Host to the BMC
2. Responses sent from the BMC to the Host in response to commands
3. Asynchronous events raised by the BMC

### General Use

Messages usually originate from the host to the BMC. There are special
cases for a back channel for the BMC to pass new information to the
host which will be discussed later.

To initiate a request the host must set a command code (see Commands) into
mailbox data register 0, and generate a sequence number (see Sequence Numbers)
to write to mailbox register data 1. After these two values, any
command-specific data should be written (see Layout). The host must then
generate an interrupt to the BMC by using bit 0 of its control register and
wait for an interrupt on the response register.  Generating an interrupt
automatically sets bit 7 of the corresponding control register. This bit can be
used to poll for messages.

On receiving an interrupt (or polling on bit 7 of its Control
Register) the BMC should read the message from the general registers
of the mailbox and perform the necessary action before responding. On
responding the BMC must ensure that the sequence number is the same as
the one in the request from the host. The BMC must also ensure that
mailbox data register 13 is a valid response code (see Responses). The
BMC should then use its control register to generate an interrupt for
the host to notify it of a response.

### Asynchronous BMC to Host Events

BMC to host communication is also possible for notification of events
from the BMC. This requires that the host have interrupts enabled on
mailbox data register 15 (or otherwise poll on bit 7 of mailbox status
register 1). On receiving such a notification the host should read
mailbox data register 15 to determine the event code which was set by the
BMC (see BMC Event notifications in Commands for detail). Events which are
defined as being able to be acknowledged by the host must be with a
BMC_EVENT_ACK command.

## High Level Protocol Flow

When a host wants to communicate with the BMC via the mbox protocol the first
thing it should do it call MBOX_GET_INFO in order to establish the protocol
version which each understands. Before this, the only other commands which are
allowed are RESET_STATE and BMC_EVENT_ACK.

After this, the host can open and close windows with the CREATE_READ_WINDOW,
CREATE_WRITE_WINDOW and CLOSE_WINDOW commands. Creating a window is how the
host requests access to a section of flash. It is worth noting that the host
can only ever have one window that it is accessing at a time - hence forth
referred to as the active window.

When the active window is a write window the host can perform MARK_WRITE_DIRTY,
MARK_WRITE_ERASED and WRITE_FLUSH commands to identify changed blocks and
control when the changed blocks are written to flash.

Independently, and at any point not during an existing mbox command
transaction, the BMC may raise asynchronous events with the host to
communicate a change in state.

### Version Negotiation

Given that a majority of command and response arguments are specified as a
multiple of block size it is necessary for the host and BMC to agree on a
protocol version as this determines the block size. In V1 it is hard coded at
4K, in V2 the BMC chooses and in V3 the host is allowed to request a specific
block size with the actual size chosen communicated back to the host as a
response argument to `MBOX_GET_INFO`. Thus the host must always call
`MBOX_GET_INFO` before any other command which specifies an argument in block
size.

When invoking `MBOX_GET_INFO` the host must provide the BMC its highest
supported version of the protocol. The BMC must respond with a protocol version
less than or equal to that requested by the host, or in the event that there is
no such value, an error code. In the event that an error is returned the host
must not continue to communicate with the BMC. Otherwise, the protocol version
returned by the BMC is the agreed protocol version for all further
communication. The host may at a future point request a change in protocol
version by issuing a subsequent `MBOX_GET_INFO` command.

### Window Management

In order to access flash contents, the host must request a window be opened at
the flash offset it would like to access with the CREATE_{READ,WRITE}_WINDOW
commands. The host may give a hint as to how much data it would like to access
or otherwise set this argument to zero. The BMC must respond with the LPC bus
address to access this window and the window size. The host must not access
past the end of the active window. On returning success to either of the create
window commands the BMC must guarantee that the window provided contains data
which correctly represents the state of flash at the time the response is given.

There is only ever one active window which is the window created by the most
recent CREATE_READ_WINDOW or CREATE_WRITE_WINDOW call which succeeded. Even
though there are two types of windows there can still only be one active window
irrespective of type. A host must not write to a read window. A host may read
from a write window and the BMC must guarantee that the window reflects what
the host has written there.

A window can be closed by calling CLOSE_WINDOW in which case there is no active
window and the host must not access the LPC window after it has been closed.
If the host closes an active write window then the BMC must perform an
implicit flush. If the host tries to open a new window with an already active
window then the active window is closed (and implicitly flushed if it was a
write window). If the new window is successfully opened then it is the new
active window, if the command fails then there is no active window and the
previously active window must no longer be accessed.

The host must not access an LPC address other than that which is contained by
the active window. The host must not use write management functions (see below)
if the active window is a read window or if there is no active window.

### Write Management

The BMC has no method for intercepting writes that occur over the LPC bus. Thus
the host must explicitly notify the BMC of where and when a write has
occurred. The host must use the MARK_WRITE_DIRTY command to tell the BMC where
within the write window it has modified. The host may also use the
MARK_WRITE_ERASED command to erase large parts of the active window without the
need to write 0xFF. The BMC must ensure that if the host
reads from an area it has erased that the read values are 0xFF. Any part of the
active window marked dirty/erased is only marked for the lifetime of the current
active write window and does not persist if the active window is closed either
implicitly or explicitly by the host or the BMC. The BMC may at any time
or must on a call to WRITE_FLUSH flush the changes which it has been notified
of back to the flash, at which point the dirty or erased marking is cleared
for the active window. The host must not assume that any changes have been
written to flash unless an explicit flush call was successful, a close of an
active write window was successful or a create window command with an active
write window was successful - otherwise consistency between the flash and memory
contents cannot be guaranteed.

The host is not required to perform an erase before a write command and the
BMC must ensure that a write performs as expected - that is if an erase is
required before a write then the BMC must perform this itself (unless the
no_erase flag is set in the MARK_WRITE_DIRTY command in which case the BMC will
blindly write without a prior erase (V3)).

The host may lock an area of flash using the MARK_LOCKED command. Any attempt
to mark dirty or erased this area of flash must fail with the LOCKED_ERROR
response code. The host may open a write window which contains a locked area
of flash however changes to a locked area of flash must never be written back
to the backing data source (i.e. that area of flash must be treated as read
only with respect to the backing store at all times). An attempt to lock an area
of flash which is not clean in the current window must fail with PARAM_ERROR.
Locked flash regions must persist across a BMC reboot or daemon restart. It is
only possible to clear the lock state through a clear_locked dbus command. (V3)

### BMC Events

The BMC can raise events with the host asynchronously to communicate to the
host a change in state which it should take notice of. The host must (if
possible for the given event) acknowledge it to inform the BMC it has been
received.

If the BMC raises a BMC Reboot event then the host must renegotiate the
protocol version so that both the BMC and the host agree on the block size.
A BMC Reboot event implies a BMC Windows Reset event.
If the BMC raises a BMC Windows Reset event then the host must
assume that there is no longer an active window - that is if there was an
active window it has been closed by the BMC and if it was a write window
then the host must not assume that it was flushed unless a previous explicit
flush call was successful.

The BMC may at some points require access to the flash and the BMC daemon must
set the BMC Flash Control Lost event when the BMC is accessing the flash behind
the BMC daemons back. When this event is set the host must assume that the
contents of the active window could be inconsistent with the contents of flash.

## Protocol Definition

### Commands

```
RESET_STATE          0x01
GET_MBOX_INFO        0x02
GET_FLASH_INFO       0x03
CREATE_READ_WINDOW   0x04
CLOSE_WINDOW         0x05
CREATE_WRITE_WINDOW  0x06
MARK_WRITE_DIRTY     0x07
WRITE_FLUSH          0x08
BMC_EVENT_ACK        0x09
MARK_WRITE_ERASED    0x0a	(V2)
GET_FLASH_NAME       0x0b	(V3)
MARK_LOCKED          0x0c	(V3)
```

### Responses

```
SUCCESS		1
PARAM_ERROR	2
WRITE_ERROR	3
SYSTEM_ERROR	4
TIMEOUT		5
BUSY		6	(V2)
WINDOW_ERROR	7	(V2)
SEQ_ERROR	8	(V2)
LOCKED_ERROR	9	(V3)
```

### Sequence Numbers

Sequence numbers are included in messages for correlation of commands and
responses. V1, V2 and V3 of the protocol permit either zero or one commands to
be in progress (yet to receive a response).

For generality, the host must generate a sequence number that is unique with
respect to the previous command (one that has received a response) and any
in-progress commands. Sequence numbers meeting this requirement are considered
valid. The BMC's response to a command must contain the same sequence number
issued by the host as found in the relevant command.

Sequence numbers may be reused in accordance with the constraints outlined
above. However, it is not an error if the BMC receives a `GET_MBOX_INFO` with an
invalid sequence number. For all other cases, the BMC must respond with
`SEQ_ERROR` if the constraints are violated. If the host receives a `SEQ_ERROR`
response it must consider any in-progress commands to have failed. The host may
retry the affected command(s) after generating a suitable sequence number.

#### Description:

SUCCESS		- Command completed successfully

PARAM_ERROR	- Error with parameters supplied or command invalid

WRITE_ERROR	- Error writing to the backing file system

SYSTEM_ERROR	- Error in BMC performing system action

TIMEOUT		- Timeout in performing action

BUSY		- Daemon in suspended state (currently unable to access flash)
		- Retry again later

WINDOW_ERROR	- Command not valid for active window or no active window
		- Try opening an appropriate window and retrying the command

SEQ_ERROR	- Invalid sequence number supplied with command

LOCKED_ERROR	- Tried to mark dirty or erased locked area of flash

### Information
- All multibyte messages are LSB first (little endian)
- All responses must have a valid return code in byte 13


### Commands in detail

Block size refers to an agreed value which is used as a unit for the
arguments of various commands and responses. Having a block size multiplier
allows us to specify larger values with fewer command and response fields.

In V1 block size is hard coded to 4K.
In V2 it is variable and must be queried with the GET_MBOX_INFO command.
In V3 the host can request a given block size however it is ultimately up to
the daemon to choose a block size which is returned as part of the GET_MBOX_INFO
command response. The host must respect the daemons choice. The ability for the
host to request a block size is provided such that it can choose an appropriate
size to be able to utilise commands which only operate at the block level.

Note that for simplicity block size must always be a power-of-2.
Block size must also be greater than or equal to 4K. This is due to the
fact that we have a 28-bit LPC address space and commands which return an
LPC address do so in 16 bits, thus we need at least a 12-bit unit to ensure
that we can specify the entire address space. This additionally allows us
to specify flash addresses of at least 256MB.

Sizes and addresses are specified in either bytes - (bytes)
					 or blocks - (blocks)
Sizes and addresses specified in blocks must be converted to bytes by
multiplying by the block size.
```
Command:
	RESET_STATE
	Added in:	V1
	Arguments:
		-
	Response:
		-
	Notes:
		This command is designed to inform the BMC that it should put
		host LPC mapping back in a state where the SBE will be able to
		use it. Currently, this means pointing back to BMC flash
		pre mailbox protocol. Final behavior is still TBD.

Command:
	GET_MBOX_INFO
	Added in:	V1
	Arguments:
		V1:
		Args 0: API version

		V2:
		Args 0: API version

		V3:
		Args 0: API version
		Args 1: Requested block size (shift)

	Response:
		V1:
		Args 0: API version
		Args 1-2: default read window size (blocks)
		Args 3-4: default write window size (blocks)

		V2:
		Args 0: API version
		Args 1-2: reserved
		Args 3-4: reserved
		Args 5: Block size as power of two (encoded as a shift)
		Args 6-7: Suggested Timeout (seconds)

		V3:
		Args 0: API version
		Args 1-2: reserved
		Args 3-4: reserved
		Args 5: Block size as power of two (encoded as a shift)
		Args 6-7: Suggested Timeout (seconds)
		Args 8: Num Allocated Flash IDs
	Notes:
		The suggested timeout is a hint to the host as to how long
		it should wait after issuing a command to the BMC before it
		times out waiting for a response. This is the maximum time
		which the BMC thinks it could take to service any command which
		the host could issue. This may be set to zero to indicate that
		the BMC	does not wish to provide a hint in which case the host
		must choose some reasonable value.

		The host may desire a specific block size and thus can request
		this by giving a hint to the daemon (may be zero). The daemon
		may use this to select the block size which it will use however
		is free to ignore it. The value in the response is the block
		size which must be used for all further requests until a new
		size is	negotiated by another call to GET_MBOX_INFO. (V3)

Command:
	GET_FLASH_INFO
	Added in:	V1
	Arguments:
		V1, V2:
		-

		V3:
		Args 0: Flash ID
	Response:
		V1:
		Args 0-3: Flash size (bytes)
		Args 4-7: Erase granule (bytes)

		V2, V3:
		Args 0-1: Flash size (blocks)
		Args 2-3: Erase granule (blocks)

Command:
	CREATE_{READ/WRITE}_WINDOW
	Added in:	V1
	Arguments:
		V1:
		Args 0-1: Requested flash offset (blocks)

		V2:
		Args 0-1: Requested flash offset (blocks)
		Args 2-3: Requested flash size to access (blocks)

		V3:
		Args 0-1: Requested flash offset (blocks)
		Args 2-3: Requested flash size to access (blocks)
		Args 4: Flash ID
	Response:
		V1:
		Args 0-1: LPC bus address of window (blocks)

		V2, V3:
		Args 0-1: LPC bus address of window (blocks)
		Args 2-3: Window size (blocks)
		Args 4-5: Flash offset mapped by window (blocks)
	Notes:
		The flash offset which the host requests access to is always
		taken from the start of flash - that is it is an absolute
		offset into flash.

		LPC bus address is always given from the start of the LPC
		address space - that is it is an absolute address.

		The requested access size is only a hint. The response
		indicates the actual size of the window. The BMC may
		want to use the requested size to pre-load the remainder
		of the request. The host must not access past the end of the
		active window.

		The flash offset mapped by the window is an absolute flash
		offset and must be less than or equal to the flash offset
		requested by the host. It is the responsibility of the host
		to use this information to access any offset which is required.

		The requested window size may be zero. In this case the
		BMC is free to create any sized window but it must contain
		atleast the first block of data requested by the host. A large
		window is of course preferred and should correspond to
		the default size returned in the GET_MBOX_INFO command.

		If this command returns successfully then the created window
		is the active window. If it fails then there is no active
		window.

Command:
	CLOSE_WINDOW
	Added in:	V1
	Arguments:
		V1:
		-

		V2:
		Args 0: Flags
	Response:
		-
	Notes:
		Closes the active window. Any further access to the LPC bus
		address specified to address the previously active window will
		have undefined effects. If the active window is a
		write window then the BMC must perform an implicit flush.

		The Flags argument allows the host to provide some
		hints to the BMC. Defined Values:
			0x01 - Short Lifetime:
				The window is unlikely to be accessed
				anytime again in the near future. The effect of
				this will depend on BMC implementation. In
				the event that the BMC performs some caching
				the BMC daemon could mark data contained in a
				window closed with this flag as first to be
				evicted from the cache.

Command:
	MARK_WRITE_DIRTY
	Added in:	V1
	Arguments:
		V1:
		Args 0-1: Flash offset to mark from base of flash (blocks)
		Args 2-5: Number to mark dirty at offset (bytes)

		V2:
		Args 0-1: Window offset to mark (blocks)
		Args 2-3: Number to mark dirty at offset (blocks)
		Args 4  : Don't Erase Before Write (V3)

	Response:
		-
	Notes:
		The BMC has no method for intercepting writes that
		occur over the LPC bus. The host must explicitly notify
		the daemon of where and when a write has occurred so it
		can be flushed to backing storage.

		Offsets are given as an absolute (either into flash (V1) or the
		active window (V2)) and a zero offset refers to the first
		block. If the offset + number exceeds the size of the active
		window then the command must not succeed.

		The host can give a hint to the daemon that is doesn't have to
		erase a flash area before writing to it by setting ARG[4]. This
		means that the daemon will blindly perform a write to that area
		and will not try to erase it before hand. This can be used if
		the host knows that a large area has already been erased for
		example but then wants to perform many small writes.

Command
	WRITE_FLUSH
	Added in:	V1
	Arguments:
		V1:
		Args 0-1: Flash offset to mark from base of flash (blocks)
		Args 2-5: Number to mark dirty at offset (bytes)

		V2:
		-

	Response:
		-
	Notes:
		Flushes any dirty/erased blocks in the active window to
		the backing storage.

		In V1 this can also be used to mark parts of the flash
		dirty and flush in a single command. In V2 the explicit
		mark dirty command must be used before a call to flush
		since there are no longer any arguments. If the offset + number
		exceeds the size of the active window then the command must not
		succeed.


Command:
	BMC_EVENT_ACK
	Added in:	V1
	Arguments:
		Args 0:	Bits in the BMC status byte (mailbox data
			register 15) to ack
	Response:
		*clears the bits in mailbox data register 15*
	Notes:
		The host should use this command to acknowledge BMC events
		supplied in mailbox register 15.

Command:
	MARK_WRITE_ERASED
	Added in:	V2
	Arguments:
		V2:
		Args 0-1: Window offset to erase (blocks)
		Args 2-3: Number to erase at offset (blocks)
	Response:
		-
	Notes:
		This command allows the host to erase a large area
		without the need to individually write 0xFF
		repetitively.

		Offset is the offset within the active window to start erasing
		from (zero refers to the first block of the active window) and
		number is the number of blocks of the active window to erase
		starting at offset. If the offset + number exceeds the size of
		the active window then the command must not succeed.

Command:
	GET_FLASH_NAME
	Added in:	V3
	Arguments:
		Args 0: Flash ID
	Response:
		Args 0   : Flash Name Length (bytes)
		Args 1-10: Flash Name / UID
	Notes:
		Describes a flash with some kind of identifier useful to the
		host system. This is typically a null-padded string.

		The length in the response is the number of response arguments
		as part of the flash name field which the host should expect to
		have been populated.

Command:
	MARK_LOCKED
	Added in:	V3
	Arguments:
		Args 0-1: Flash offset to lock (blocks)
		Args 2-3: Number to lock at offset (blocks)
		Args 4: Flash ID
	Response:
		-
	Notes:
		Lock an area of flash so that the host can't mark it dirty or
		erased. If the requested area is within the current window and
		that area is currently marked dirty or erased then this command
		must fail with PARAM_ERROR.

```

### BMC Events in Detail:

If the BMC needs to tell the host something then it simply
writes to Byte 15. The host should have interrupts enabled
on that register, or otherwise be polling it.

#### Bit Definitions:

Events which must be ACKed:
```
0x01: BMC Reboot
0x02: BMC Windows Reset (V2)
```

Events which cannot be ACKed (BMC will clear when no longer
applicable):
```
0x40: BMC Flash Control Lost (V2)
0x80: BMC MBOX Daemon Ready (V2)
```

#### Event Description:

Events which must be ACKed:
The host should acknowledge these events with BMC_EVENT_ACK to
let the BMC know that they have been received and understood.
```
0x01 - BMC Reboot:
	Used to inform the host that a BMC reboot has occurred.
	The host must perform protocol version negotiation again and
	must assume it has no active window. The host must not assume
	that any commands which didn't respond as such succeeded.
0x02 - BMC Windows Reset: (V2)
	The host must assume that its active window has been closed and
	that it no longer has an active window. The host is not
	required to perform protocol version negotiation again. The
	host must not assume that any commands which didn't respond as such
	succeeded.
```

Events which cannot be ACKed:
These events cannot be acknowledged by the host and a call to
BMC_EVENT_ACK with these bits set will have no effect. The BMC
will clear these bits when they are no longer applicable.
```
0x40 - BMC Flash Control Lost: (V2)
	The BMC daemon has been suspended and thus no longer
	controls access to the flash (most likely because some
	other process on the BMC required direct access to the
	flash and has suspended the BMC daemon to preclude
	concurrent access).
	The BMC daemon must clear this bit itself when it regains
	control of the flash (the host isn't able to clear it
	through an acknowledge command).
	The host must not assume that the contents of the active window
	correctly reflect the contents of flash while this bit is set.
0x80 - BMC MBOX Daemon Ready: (V2)
	Used to inform the host that the BMC daemon is ready to
	accept command requests. The host isn't able to clear
	this bit through an acknowledge command, the BMC daemon must
	clear it before it terminates (assuming it didn't
	terminate unexpectedly).
	The host should not expect a response while this bit is
	not set.
	Note that this bit being set is not a guarantee that the BMC daemon
	will respond as it or the BMC may have crashed without clearing
	it.
```
