# Phosphor Blob Transfer Interface

This document describes the commands implementing a generic blob transfer
interface. This document does not specify how blobs are stored; that is left up
to blob-specific implementations. Introduction This mechanism supports reading
and writing from a generic blob store. This mechanism can be leveraged to
support firmware upgrades,

The interface defined in this document supports:

*   Enumerating blobs
*   Opening a blob for reading or writing
*   Writing blob content
*   Committing a blob
*   Polling until the blob has been committed
*   Closing a blob
*   Reading blob content
*   Deleting a blob

Some blobs will only support a subset of these operations. For example, firmware
cannot generally be read, as the firmware file is not persisted on the BMC after
it has been activated.

This specification supports IPMI as a transport layer. This can be quite slow,
however; IPMI transport speeds range from 23 kbps to 256 kbps. LPC/P2A ("PCI to
Aspeed") MMIO bridges are currently unsupported. Blob Identifiers Blobs are
identified by NULL-terminated C strings. This protocol supports
implementation-specific blob identifiers; some blobs may have single well-known
names, while others may be defined only by a prefix, with the client specifying
the rest of the blob name. For example, "/g/bmc_firmware" may represent a single
well-known blob that supports BMC firmware updates, whereas "/g/skm/[N]" may
represent an arbitrary number of SKM keys, with the index specified on the host
when opening the blob.

Blob identifiers are limited based on the maximum size of the IPMI packet. This
maximum size is platform-dependent; theoretically a packet could be up to 256
bytes, but some hardware only supports packets up to 64 bytes.

If an identifier is malformed, e.g. does not have a trailing NUL-byte or is
otherwise unrecognizable by the BMC, an error is returned.

The OEM Number to use with these commands is
[openbmc oen](https://github.com/openbmc/phosphor-host-ipmid/blob/194375f2676715a0e0697bab63234a4efe39fb96/include/ipmid/iana.hpp#L12)
49871.

## Commands

The following details each subcommand in the blob spec. In the following, any
reference to the command body starts after the 3 bytes of OEM header, the 1-byte
subcommand code, and (in the cases where the command body is non-empty) a
two-byte CRC over all data that follows in the body.

All non-empty responses should lead with a two-byte CRC. The CRC algorithm is
CRC-16-CCITT (poly 0x1021).

All multi-byte values are encoded as little-endian. All structs are assumed
packed. Success codes are returned via the "completion code" field from the IPMI
spec.

### BmcBlobGetCount (0)

The `BmcBlobGetCount` command expects to receive an empty body. The BMC will
return the number of enumerable blobs:

```
struct BmcBlobCountRx {
    uint16_t crc16;
    uint32_t blob_count;
};
```

### BmcBlobEnumerate (1)

The `BmcBlobEnumerate` command expects to receive a body of:

```
struct BmcBlobEnumerateTx {
    uint16_t crc16;
    uint32_t blob_idx; /* 0-based index of blob to retrieve. */
};
```

The BMC will return the corresponding blob identifier:

```
struct BmcBlobEnumerateRx {
    uint16_t crc16;
    char     blob_id[];
};
```

Note that the index for a given blob ID is not expected to be stable across a
long term. Callers are expected to call `BmcBlobGetCount`, followed by N calls
to `BmcBlobEnumerate`, to collect all blob IDs. Callers can then call
`BmcBlobStat` with each blob ID. If this process is interleaved with Open or
Delete calls that modify the number of enumerable blobs, this operation will be
subject to concurrent modification issues.

### BmcBlobOpen (2)

The BmcBlobOpen command expects to receive a body of:

```
struct BmcBlobOpenTx {
    uint16_t crc16;
    uint16_t flags;
    char     blob_id[]; /* Must correspond to a valid blob. */
};
```

The flags field allows the caller to specify whether the blob is being opened
for reading or writing:

```
enum BmcBlobOpenFlagBits {
    READ = 0,
    WRITE = 1,
    <bits 2-7 reserved>
    <bits 8-15 given blob-specific definitions>
};
```

If the `WRITE` flag is specified, the BMC will mark the specified blob as "open
for writing". Optional blob-specific behavior: if the blob has been open for
more than one minute without any activity, the existing session will be torn
down and the open will succeed. Alternatively, blobs may allow multiple write
sessions to be active at once.

The BMC allocates a unique session identifier, and internally maps it to the
blob identifier.

```
struct BmcBlobOpenRx {
    uint16_t crc16;
    uint16_t session_id;
};
```

### BmcBlobRead (3)

The BmcBlobRead command is used to read blob data. It expects to receive a body
of:

```
struct BmcBlobReadTx {
    uint16_t crc16;
    uint16_t session_id; /* Returned from BmcBlobOpen. */
    uint32_t offset; /* The byte sequence start, 0-based. */
    uint32_t requested_size; /* The number of bytes requested for reading. */
};
```

Blob handlers may require the blob’s "state" to equal `OPEN_R` before reading is
successful.

```
struct BmcBlobReadRx {
    uint16_t crc16;
    uint8_t  data[];
};
```

Immediately following this structure are the bytes being read. The number of
bytes transferred is the size of the response body less the OEN ("OEM number")
(3 bytes), sub-command (1 byte), and the structure size (4 bytes). If no bytes
are transferred, the CRC is still sent.

If the BMC cannot return the number of requested bytes, it simply returns the
number of bytes available for reading. If the host tries to read at an invalid
offset or if the host tries to read at the end of the blob, an empty successful
response is returned; e.g., data is empty.

### BmcBlobWrite (4)

The `BmcBlobWrite` command expects to receive a body of:

```
struct BmcBlobWriteTx {
    uint16_t crc16;
    uint16_t session_id; /* Returned from BmcBlobOpen. */
    uint32_t offset; /* The byte sequence start, 0-based. */
    uint8_t  data[];
};
```

Immediately following this structure are the bytes to write. The length of the
entire packet is variable and handled at a higher level, therefore the number of
bytes to write is the size of the command body less the OEN and sub-command (4
bytes) and less the structure size (10 bytes). It is assumed that all writes are
sequential, and begin at offset zero.

On success it will return a success completion code.

### BmcBlobCommit (5)

The `BmcBlobCommit` command expects to receive a body of:

```
struct BmcBlobCommitTx {
    uint16_t crc16;
    uint16_t session_id; /* Returned from BmcBlobOpen. */
    uint8_t  commit_data_len;
    uint8_t  commit_data[]; /* Optional blob-specific commit data. */
};
```

Each blob defines its own commit behavior. A BMC firmware blob may be verified
and saved to the filesystem. Commit operations may require additional data,
which would be provided following the structure in the IPMI packet.

The commit operation may exceed the IPMI timeout duration of ~5 seconds
(implementation dependant). Callers are expected to poll on `BmcBlobSessionStat`
or `BmcBlobStat` (as appropriate) until committing has finished. To address race
conditions, blobs should not allow concurrent sessions that modify state.

On success, the BMC returns success completion code.

### BmcBlobClose (6)

The `BmcBlobClose` command must be called after commit-polling has finished,
regardless of the result. It expects to receive a body of:

```
struct BmcBlobCloseTx {
    uint16_t crc16;
    uint16_t session_id; /* Returned from BmcBlobOpen. */
};
```

The BMC marks the specified blob as closed. On success, the BMC returns a
success completion code.

### BmcBlobDelete (7)

The `BmcBlobDelete` command is used to delete a blob. Not all blobs will support
deletion. This command expects to receive a body of:

```
struct BmcBlobDeleteTx {
    uint16_t crc16;
    char     blob_id[]; /* Must correspond to a valid blob. */
};
```

If the operation is supported, the blob is deleted. On success, the BMC returns
a success completion code. This command will fail if there are open sessions for
the blob.

### BmcBlobStat (8)

The `BmcBlobStat` command is used to retrieve statistics about a blob. Not all
blobs must support this command; this is only useful when blob_id semantics are
more useful than session IDs. This command expects to receive a body of:

```
struct BmcBlobStatTx {
    uint16_t crc16;
    char     blob_id[]; /* Must correspond to a valid blob. */
};
```

The BMC returns the following data:

```
struct BmcBlobStatRx {
    uint16_t crc16;
    uint16_t blob_state;
    uint32_t size; /* Size in bytes of the blob. */
    uint8_t  metadata_len;
    uint8_t  metadata[]; /* Optional blob-specific metadata. */
};
```

The blob_state field is a bit field with the following flags:

```
enum BmcBlobStateFlagBits {
    OPEN_R = 0,
    OPEN_W = 1,
    COMMITTING = 2,
    COMMITTED = 3,
    COMMIT_ERROR = 4,
    <bits 5-7 reserved>
    <bits 8-15 given blob-specific definitions>
};
```

If the state is `COMMITTING`, the blob is not currently available for reading or
writing. If the state is `COMMITTED`, the blob may be available for reading.

The size field may be zero if the blob does not support reading.

Immediately following this structure are optional blob-specific bytes. The
number of bytes transferred is the size of the response body less the OEN and
sub-command and less the structure size. The metadata must fit in a single IPMI
packet, which has a platform-dependent maximum size. (For reference, Aspeed
supports 64 bytes max.)

If the blob is open or committed but has been inactive for longer than the
specified activity timeout, the blob is closed, and blob_status is set to
`CLOSED` in the response.

### BmcBlobSessionStat (9)

The `BmcBlobSessionStat` command returns the same data as `BmcBlobStat`.
However, this command operates on sessions, rather than blob IDs. Not all blobs
must support this command; this is only useful when session semantics are more
useful than raw blob IDs.

```
struct BmcBlobSessionStatTx {
    uint16_t crc16;
    uint16_t session_id; /* Returned from BmcBlobOpen. */
};
```

```
struct BmcBlobSessionStatRx {
    uint16_t crc16;
    uint16_t blob_state;
    uint32_t size; /* Size in bytes of the blob. */
    uint8_t  metadata_size;
    uint8_t  metadata[]; /* Optional blob-specific metadata. */
};
```

### BmcBlobWriteMeta (10)

The `BmcBlobWriteMeta` command behaves like `BmcBlobWrite`. Its purpose is
blob-specific, and not all blobs must support it.

The `BmcBlobWriteMeta` command expects to receive a body of:

```
struct BmcBlobWriteMetaTx
{
    uint16_t crc16;
    uint16_t session_id; /* Returned from BmcBlobOpen. */
    uint32_t offset; /* The byte sequence start, 0-based. */
    uint8_t  data[];
};
```

Immediately following this structure are the bytes to write. The length of the
entire packet is variable and handled at a higher level, therefore the number of
bytes to write is the size of the command body less the OEN and sub-command (4
bytes) and less the structure size.

On success it will return a success completion code.

## Idempotent Commands

The IPMI transport layer is somewhat flaky. Client code must rely on a
"send-with-retries" strategy to ensure that commands make their way from the
host to the BMC. Commands can fail if the BMC is busy with other commands.

It is possible that an IPMI command successfully invokes the BMC-side handler,
but that the response does not successfully make its way back to the host. In
this case, the host may decide to retry the command. Thus, command handlers
should be idempotent where possible; duplicate calls should return the same
value if repeated, while avoiding potentially destructive side-effects.

## Stale Sessions

Each blob type will define an operation for cleansing stale sessions. This could
involve scrubbing secrets or freeing buffers. A function will be provided that
will scan over each open session, to determine which if any sessions have been
open for longer than 10 minutes with no activity. For each session, the
associated blob type’s cleansing routine will be invoked, and the associated
session ID will be freed. This function will be invoked from the `BmcBlobOpen`
command handler, though not more than once every minute.

## Handler Calling Contract

The blob manager provides the following calling contract guarantees:

*   The blob manager will only call `open()` on your handler if the handler
    responds that they can handle the path.
*   The blob manager will only call a session-based command against your handler
    if that session is already open (e.g. `stat()` or `commit()`).
    *   The caveat is the open command where the session is provided to the
        handler within the call.
*   The blob manager will only call `delete()` on a blob if there are no open
    sessions to that blob id.
