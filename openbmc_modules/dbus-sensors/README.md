# dbus-sensors

dbus-sensors is a collection of sensor applications that provide the
xyz.openbmc_project.Sensor collection of interfaces. They read sensor values
from hwmon, d-bus, or direct driver access to provide readings. Some advance
non-sensor features such as fan presence, pwm control, and automatic cpu
detection (x86) are also supported.

## key features

- runtime re-configurable from d-bus (entity-manager or the like)

- isolated: each sensor type is isolated into its own daemon, so a bug in one
  sensor is unlikely to affect another, and single sensor modifications are
  possible

- async single-threaded: uses sdbusplus/asio bindings

- multiple data inputs: hwmon, d-bus, direct driver access
