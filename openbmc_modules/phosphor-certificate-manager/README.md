# phosphor-certificate-manager
Certificate management allows to replace the existing certificate and private
key file with another (possibly CA signed) Certificate key file. Certificate
management allows the user to install both the server and client certificates.

## To Build
```
To build this package, do the following steps:

    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS}
    3. make

To clean the repository run `./bootstrap.sh clean`.
```

## To Run
Multiple instances of `phosphor-certificate-manager` are usually run on the bmc
to support management of different types of certificates.
```
Usage: ./phosphor-certificate-manager [options]
Options:
    --help            Print this menu
    --type            certificate type
                      Valid types: client,server,authority
    --endpoint        d-bus endpoint
    --path            certificate file path
    --unit=<name>     Optional systemd unit need to reload
```

### Https certificate management
**Purpose:** Server https certificate
```bash
./phosphor-certificate-manager --type=server --endpoint=https \
    --path=/etc/ssl/certs/https/server.pem --unit=bmcweb.service
```

### CA certificate management
**Purpose:** Client certificate validation
```bash
./phosphor-certificate-manager --type=authority --endpoint=ldap \
    --path=/etc/ssl/certs/authority --unit=bmcweb.service
```

### LDAP client certificate management
**Purpose:** LDAP client certificate validation
```bash
./phosphor-certificate-manager --type=client --endpoint=ldap \
    --path=/etc/nslcd/certs/cert.pem
```

## D-Bus Interface
`phosphor-certificate-manager` is an implementation of the D-Bus interface
defined in [this document](https://github.com/openbmc/phosphor-dbus-interfaces/blob/a3d0c212a1e734a77fbaf11c7561c59e59d514da/xyz/openbmc_project/Certs/README.md).

D-Bus service name is constructed by
"xyz.openbmc_project.Certs.Manager.{Type}.{Endpoint}"
and D-Bus object path is constructed by
"/xyz/openbmc_project/certs/{type}/{endpoint}".

Take https certificate management as an example.
```bash
./phosphor-certificate-manager --type=server --endpoint=https \
    --path=/etc/ssl/certs/https/server.pem --unit=bmcweb.service
```
D-Bus service name is "xyz.openbmc_project.Certs.Manager.Server.Https" and
D-Bus object path is "/xyz/openbmc_project/certs/server/https".

## Usage in openbmc/bmcweb
OpenBMC [bmcweb](https://github.com/openbmc/bmcweb) exposes various [REST APIs](https://github.com/openbmc/bmcweb/blob/master/redfish-core/lib/certificate_service.hpp)
for certificate management on the BMC, which leverages functionalities of
`phosphor-certificate-manager` via D-Bus.

