# phosphor-bmc-code-mgmt
Phosphor BMC Code Management provides a set of system software management
applications. More information can be found at
[Software Architecture](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Software/README.md)

## To Build
To build this package, do the following steps:

1. `meson build`
2. `ninja -C build`

To clean the repository run `rm -r build`.
