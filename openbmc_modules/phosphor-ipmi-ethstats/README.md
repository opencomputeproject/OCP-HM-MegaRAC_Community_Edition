### Ethernet Statistics Command (0x30)

There is the need to read a specific ethernet-level statistic value from the BMC.  This is driven primarily to detect link errors that require hardware swaps during manufacturing.

This command will be well structured such that there is a request and response which mirror to some extent.

The request will specify the ethernet interface by name, as a length-prepended string, and the field they're requesting by identifier (an unsigned byte).

If the command is not supported, the IPMI OEM handler will respond accordingly, however, if the field is not supported or not recognized, the command will return 0xcc (invalid field).

The current ethernet statistics available (all future additions must append):

|Identifier |Human Readable Name
|-----------|--------------------
|0          |rx_bytes
|1          |rx_compressed
|2          |rx_crc_errors
|3          |rx_dropped
|4          |rx_errors
|5          |rx_fifo_errors
|6          |rx_frame_errors
|7          |rx_length_errors
|8          |rx_missed_errors
|9          |rx_nohandler
|10         |rx_over_errors
|11         |rx_packets
|12         |tx_aborted_errors
|13         |tx_bytes
|14         |tx_carrier_errors
|15         |tx_compressed
|16         |tx_dropped
|17         |tx_errors
|18         |tx_fifo_errors
|19         |tx_heartbeat_errors
|20         |tx_packets
|21         |tx_window_errors

Request

|Byte(s) |Value |Data
|--------|------|----
|0x00    |Statistic ID |The identifier of the desired statistic.
|0x01    |Length       |Length of string (not including null termination).
|0x02..  |The name     |The string, not null-terminated.

Response

|Byte(s) |Value  |Data
|--------|-------|----
|0x00    |Stat ID|The identifier of the desired statistic.
|0x01....|Uint64 |The value.  Because these are counters we don't anticipate negative values, and we don't expect overflow.
