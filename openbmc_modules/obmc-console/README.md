## To Build
Note: In addition to a toolchain and autoconf tools, this requires `autotools-archive` to be installed.

To build this project, run the following shell commands:

```
./bootstrap.sh
./configure ${CONFIGURE_FLAGS}
make
```

To fully clean the repository, run:
```
./bootstrap.sh clean
```

## To Run Server
Running the server requires a serial port (e.g. /dev/ttyS0):

```
touch obmc-console.conf
./obmc-console-server --config obmc-console.conf ttyS0
```

## To Connect Client
To connect to the server, simply run the client:

```
./obmc-console-client
```

To disconnect the client, use the standard `~.` combination.


## Underlying design

This shows how the host UART connection is abstracted within the BMC as a Unix
domain socket.

```
               +--------------------------------------------------------------------------------------------+
               |                                                                                            |
               |       obmc-console-client      unix domain socket         obmc-console-server              |
               |                                                                                            |
               |     +---------------------+                           +------------------------+           |
               |     | client.2200.conf    |  +---------------------+  | server.ttyVUART0.conf  |           |
           +---+--+  +---------------------+  |                     |  +------------------------+  +--------+-------+
Network    | 2200 +-->                     +->+ @obmc-console.host0 +<-+                        <--+ /dev/ttyVUART0 |   UARTs
           +---+--+  | socket-id = "host0" |  |                     |  | socket-id = "host0"    |  +--------+-------+
               |     |                     |  +---------------------+  |                        |           |
               |     +---------------------+                           +------------------------+           |
               |                                                                                            |
               |                                                                                            |
               |                                                                                            |
               +--------------------------------------------------------------------------------------------+
```

This supports multiple independent consoles.  The socket-id is a unique
portion for the unix domain socket created by the obmc-console-server
instance. The server needs to know this because it needs to know what to name
the pipe; the client needs to know it as it needs to form the abstract socket
name to which to connect.
