Phosphor OpenBMC and IIO devices

Phosphor OpenBMC currently does not provide an IIO device to DBUS bridge in the
same way that hwmon devices are bridged by the phosphor-hwmon-readd application.

Until a daemon can be written, the hwmon-iio bridge driver can be used with
the phosphor-hwmon-readd application, with the limitation that only a single
iio channel can be configured per iio-hwmon platform instance.  Typically
device trees are setup with all the iio-channels under a single iio-hwmon
platform device - doing this will result in undefined behavior from
phosphor-hwmon-readd.

If a true IIO bridging daemon becomes available in the future, phosphor-hwmon-readd
will not support hwmon-iio bridge devices in any capacity.
