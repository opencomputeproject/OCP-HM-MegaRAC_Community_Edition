# Sketch of OpenBMC OEM message formats

This document describes OEM Requests to be supported using the OpenBMC
OEM Number.

### What's in the box?

* Briefly recap OEM Extension layout as described in IPMI Specification.
* Enumerate Command codes allocated for use with the OpenBMC OEM Number.
* For each such code, describe the associated messages, including
  representation, function, meaning, limits, and so on.

### OEM Extensions, Block Transfer Transport Example.

This table and the next briefly recap OEM Extension messages as
described in the
[IPMI Specification](
http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-second-gen-interface-spec-v2-rev1-1.html).
To keep it both simple and concrete, only BT Tansport is described.
Please consult that specification for the full story.

#### OEM Request

Per section 11.1 of IPMI Spec

| Bytes   | Bits | Spec ID | Value | Description
| :---:   | ---: | :------ | :---: | :----------
| 0       |      | Length* |   -   | Number of bytes to follow in this request
| 1       |  2:7 | NetFn   | 0x2E  | OEM Request
| 1       |  0:1 | LUN     |   -   | Allow any LUN
| 2       |      | Seq*    |   -   | Per section 11.3 of IPMI Spec
| 3       |      | Cmd     |   -   | Table 3 - OpenBMC Cmd Codes
| 4 ~ 6   |      | OEN     | 49871 | OEM Enterprise Number
| 2 ~ n+6 |      | Data    |   -   | n data bytes, encoding depends on Cmd

Notes:

* Length and Seq are specific to BT transport - other transports may not
  have them.

* Serialize numbers larger than 1 byte LSB first - e.g.,
  represent OEM Enterprise Number 49871 as `{0xcf, 0xc2, 0x00}`

#### OEM Response

Per section 11.2 of IPMI Spec

| Bytes   | Bits | Spec ID | Value | Description
| :---:   | ---: | :------ | :---: | :----------
| 0       |      | Length* |   -   | Number of bytes to follow in this response
| 1       | 2:7  | NetFn   | 0x2F  | OEM Response
| 1       | 0:1  | LUN     |   -   | LUN of request to which this is a response
| 2       |      | Seq*    |   -   | Seq of request to which this is a response
| 3       |      | Cmd     |   -   | Cmd code of request to which this is a response
| 4       |      | CC      |   -   | Completion code, Section 5.2 of IPMI Spec v2.0
| 5 ~ 7   |      | OEN     | 49871 | OEM Enterprise Number
| 8 ~ n+7 |      | Data    |   -   | n data bytes, encoding depends on Cmd

### OpenBMC OEM Cmd Codes

```
/*
 * This is the OpenBMC IANA OEM Number
 */
constexpr OemNumber obmcOemNumber = 49871;
```

These are the Command codes allocated for use with the OpenBMC OEM Number.

| Cmd     | Identifier    | Description
| :---:   | :---          | :---
| 0       | -             | Reserved
| 1       | gpioCmd       | GPIO Access
| 2       | i2cCmd        | I2C Device Access
| 3       | flashCmd      | Flash Device Access
| 4       | fanManualCmd  | Manual Fan Controls
| 5 ~ 255 |       -       | Unallocated

### I2C Device Access (Command 2)

The next subsections describe command and response messages supporting
the I2C Device Access extension OpenBMC OEM extension (command code 2).

#### I2C Request Message - Overall

| Bytes   | Bits | Identifier    | Description
| :---:   | :--- | :---          | :---
| 0       |      | bus           | Logical I2C bus.
| 1       |      | xferFlags     | Flags for all steps.
|         | 7    | I2cFlagUsePec | 1 => use PEC - see note.
|         | 6:0  |               | Reserved(0)
| 2 ~ n-1 |      |               | Step sequence - see next.

Notes

* Total length of step sequence must exactly fill request.

* Intent is to handle [Linux Kernel SMBus Protocol](https://www.kernel.org/doc/Documentation/i2c/smbus-protocol),
with com generalized to m byte sequence - e.g., at24c64 uses 2 address bytes,
and n bytes of received data,
rather than specific operations for various sizes.

* Goal is to support SMBus v2 32-byte data block length limit;
but easily supports new 4 and 8 byte transfers added for
[SMBus v3](http://smbus.org/specs/SMBus_3_0_20141220.pdf).

* PEC refers to SMBus Packet Error Check.

* SMBus address resolution, alerts, and non-standard protocols not supported.
So for example, there is no way to insert a stop command within a transfer.

* Depending on options, i2cdetect uses either quick write or 1 byte read;
default is 1-byte read for eeprom/spd memory ranges, else quick write.

#### I2C Request Message - Step Properties

| Bytes | Bits  | Identifier     | Description
| :---: | :---: | :---           | :---
| 0     |       | devAndDir
|       |  7:1  | dev            | 7-bit I2C device address.
|       |  0    | isRead         | 1 = read, 0 = write.
| 1     |       | stepFlags
|       |  7    | i2cFlagRecvLen | 1 if block read, else regular; see table.
|       |  6    | i2cFlagNoStart | 1 to suppress I2C start.
|       |  5:0  |                | Reserved(0)
| 2     |       | p              | Count parameter; see table
| 3:m+2 |       | wrPayload      | Nonempty iff p supplies nonzero m; see table.

##### Allowed step property combinations

| is_read | RecvLen | p     | Size | Description
| :---:   | :---:   | :---: | :--- | :---
| 0       | 0       | 0     | 3    | Quick write 0; same as write m=0 bytes.
| 0       | 0       | m     | m+3  | Consume and write next m bytes.
| 0       | 1       | -     | 3    | Reserved.
| 1       | 0       | 0     | 3    | Quick write 1; same as read n=0 bytes.
| 1       | 0       | n     | 3    | Read n bytes.
| 1       | 1       | -     | 3    | Read count from device.

Notes

* All other combinations are reserved.

* RecvLen corresponds to Linux
[I2C_M_RECV_LEN message flags bit](http://elixir.free-electrons.com/linux/v4.10.17/source/include/uapi/linux/i2c.h#L78)

* Transfers include byte count in SMBUS block modes, and
  PEC byte when selected. Allows for use with SMBUS compatibility layer.

* In case of write steps, wrPayload may include:
    * register address byte or bytes;
    * count byte if implementing SMBUS block or call operation;
    * up to 32 bytes of logical payload; and
    * PEC byte.

#### I2C Response Message

| Bytes | Identifier | Size   | Description
| :---: | :---       | :---:  | :---
| 0:n-1 | rd_data    | 0 ~ 34 | byte sequence read

Notes

* Maximum logical payload is 32.

* In the unlikely event a transfer specifies multiple read steps,
  all bytes read are simply concatenated in the order read.

* However, maximum reply limit is sized for the largest single read.

* RecvLen case w/ PEC can return up to 34 bytes:
    count + payload + PEC
