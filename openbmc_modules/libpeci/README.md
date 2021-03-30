# libpeci

libpeci is a library that provides various APIs to interface with the IOCTLs
provided by the PECI driver in the OpenBMC kernel.
Currently available here:

`https://github.com/openbmc/linux/blob/dev-5.4/include/uapi/linux/peci-ioctl.h`

## peci_cmds
This repo also includes a peci_cmds command-line utility with functions that
map to the libpeci APIs. It can be used to test PECI functionality across
the library, driver, and hardware.
