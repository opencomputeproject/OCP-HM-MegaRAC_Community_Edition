phosphor-msl-verify

phosphor-msl-verify is a "oneshot" application for basic minimum ship level
[(MSL)](https://github.com/openbmc/phosphor-dbus-interfaces/xyz/openbmc_project/control/README.msl)
verification.

The application first determines if MSL validation is disabled and if not,
searches the D-Bus object namespace for any MeetsMSL interfaces and exits with
non-zero status if any inventory items implementing the interface are found
that do not meet the MSL.
