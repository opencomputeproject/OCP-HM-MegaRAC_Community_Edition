# Service Management

## Overview
Applications must use service manager daemon to configure services like
phosphor-ipmi-net, bmcweb, obmc-console etc in the system, instead of directly
controlling the same using 'systemd' or 'iptables'. This way client
applications doesn't need to change to configure services, when the
implementations differ. The list of services supported are
`"phosphor-ipmi-net", "bmcweb", "phosphor-ipmi-kcs", "start-ipkvm",
"obmc-console"`.

## Implementation Details

Service manager daemon will create D-Bus objects for configurable services
in the system under the object path `/xyz/openbmc_project/control/service`. For
each instance of the service there will be a D-Bus object
`/xyz/openbmc_project/control/service/<service-name>`.
For example, if there are two instances of `phosphor-ipmi-net` then there
will be two D-Bus objects
`/xyz/openbmc_project/control/service/phosphor_2dipmi_2dnet_40eth0`
and `/xyz/openbmc_project/control/service/phosphor_2dipmi_2dnet_40eth1`.
The D-Bus object manages both the associated service and socket unit files.
The D-Bus object implements the interface
`xyz.openbmc_project.Control.Service.Attributes`. Network services like bmcweb,
phosphor-ipmi-net also implements the
`xyz.openbmc_project.Control.Service.SocketAttributes` interface.

In order to update the property value of a service, `override.conf` file under
`/etc/systemd/system/<Service unit name>/` is updated and the unit is restarted
through `org.freedesktop.systemd1`.

#### xyz.openbmc_project.Control.Service.Attributes interface
##### properties

* Enabled - indicates whether the service is enabled or disabled, `true`
            indicates the service will be started on the next boot and `false
            indicates that service will not be started on the next boot. This
            property can be used to change the service behaviour on the next
            boot, `true` to start the service on the next boot and `false` to
            not start the service on the next boot. Even if the service is
            disabled, on the next boot it can be started if there are other
            service dependencies to satisy. The service cannot be enabled if the
            service is masked.

* Masked  - indicates whether the service is masked, `true` indicates the
            service is permanently disabled and `false` indicates the service
            is enabled. If the property is set to `true`, then the service is
            permanently disabled and the service is stopped. If the property
            is set to `false` then the service is enabled and starts running.

* Running - indicates the current state of the service, `true` if the service
            is running and `false` if the service is not running. This property
            can be used to change the running state of the service, to start the
            service set to `true` and to stop the service set to `false`. The
            service cannot be started if the service is Masked.

#### xyz.openbmc_project.Control.Service.SocketAttributes interface
##### properties

* Port - Port number to which the service is configured to listen, if
         applicable for service. Services like obmc-console will not
         implement this interface.

## Usage

To permanently disable a service the `Masked` property under the interface
`xyz.openbmc_project.Control.Service.Attributes` needs to be set to `true` and
vice versa to enable a service.

RMCP+ port number can be modified from the default port number 623 to a custom
one by updating the `Port` property value under the interface
`xyz.openbmc_project.Control.Service.SocketAttributes`.


