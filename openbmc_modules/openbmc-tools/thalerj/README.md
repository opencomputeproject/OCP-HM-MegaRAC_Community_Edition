# openbmctool Documentation

This provides documentation beyond what is in the tool's help text.

## Connecting to a system

An IP address or hostname, password, and username are required for
connecting to a BMC.

These are passed in with the following options:
- `-H`: The hostname or IP for the BMC
- `-U`: The username
One of the password options:
- `-P`: The password, provided in-line
- `-A`: Prompt for a password
- `-E`: Take the password from envvar OPENBMCTOOL_PASSWORD

## Enabling and Disabling Local BMC User Accounts

The local user accounts on the BMC, such as root, can be disabled, queried,
and re-enabled with the 'local_users' sub-command.

Important:  After disabling local users, an LDAP user will need to be used
for further interaction with the BMC, including if using openbmctool to
enable local users again.

To view current local user account status:
```
openbmctool <connection options> local_users queryenabled
```

To disable all local user accounts:
```
openbmctool <connection options> local_users disableall
```

To re-enable all local user accounts:
```
openbmctool <connection options> local_users enableall
```

## Remote logging via rsyslog

The BMC has the ability to stream out local logs (that go to the systemd journal)
via [rsyslog](https://www.rsyslog.com/).

The BMC will send everything. Any kind of filtering and appropriate storage will
have to be managed on the rsyslog server. Various examples are available on the
internet. Here are few pointers:
https://www.rsyslog.com/storing-and-forwarding-remote-messages/
https://www.rsyslog.com/doc/rsyslog%255Fconf%255Ffilter.html
https://www.thegeekdiary.com/understanding-rsyslog-filter-options/

### Configuring rsyslog server for remote logging

```
openbmctool <connection options> logging remote_logging_config -a <IP address> -p <port>
```

The IP address and port to be provided are of the remote rsyslog server.
Once this command is run, the remote rsyslog server will start receiving logs
from the BMC.

Hostname can be specified instead of IP address, if DNS is configured on the BMC.

### Disabling remote logging

```
openbmctool <connection options> logging remote_logging disable
```

It is recommended to disable remote logging before switching remote logging from
an existing remote server to a new one (i.e before re-running the remote_logging_config
option).

### Querying remote logging config

```
openbmctool <connection options> logging remote_logging view
```

This will print out the configured remote rsyslog server's IP address and port,
in JSON format.

## BMC Certificate management

Certificate management allows replacing the existing certificate and private
key file with another (possibly certification Authority (CA) signed)
certificate and private key file. Certificate management allows the user to
install server, client and root certificates.

### Update HTTPS server certificate
```
openbmctool <connection options> certificate update server https -f <File>
```
File: The [PEM](https://en.wikipedia.org/wiki/Privacy-Enhanced_Mail) file
      containing both certificate and private key.

### Update LDAP client certificate
```
openbmctool <connection options> certificate update client ldap -f <File>
```
File: The PEM file containing both certificate and private key.

### Update LDAP root certificate
```
openbmctool <connection options> certificate update authority ldap -f <File>
```
File: The PEM file containing only certificate.


### Delete HTTPS server certificate
```
openbmctool <connection options> certificate delete server https
```
Deleting a certificate will create a new self-signed certificate and will
install the same.

### Delete LDAP client certificate
```
openbmctool <connection options> certificate delete client ldap
```

### Delete LDAP root certificate
```
openbmctool <connection options> certificate delete authority ldap
```
Deleting the root certificate can cause an LDAP service outage. Please refer to
the LDAP documentation before using this command.

## BMC LDAP Configuration

In BMC, LDAP is used for remote authentication. BMC doesn't support remote user-management functionality.

BMC supports secure/non-secure LDAP configuration.

### Create LDAP Configuration

#### NonSecure
```
openbmctool.py <connection options> ldap enable --uri="ldap://<ldap server IP/hostname>" --bindDN=<bindDN> --baseDN=<basDN> --bindPassword=<bindPassword> --scope="sub/one/base" --serverType="OpenLDAP/ActiveDirectory"

```
NOTE: configuring FQDN (fully qualified domain name/ hostname) in the "uri"
parameter requires that DNS server be configured on the BMC.

NOTE: Currently, openbmctool doesn't support configuring the DNS server on the
BMC.

#### Secure
```
openbmctool.py <connection options> ldap enable --uri="ldaps://<ldap server IP/hostname>" --bindDN=<bindDN> --baseDN=<basDN> --bindPassword=<bindPassword> --scope="sub/one/base" --serverType="OpenLDAP/ActiveDirectory"

```
NOTE:
a) It is quite common to encounter the following error when running the
openbmctool.py command string shown above:

xyz.openbmc_project.Common.Error.NoCACertificate

This error means that the BMC client needs to verify that the LDAP server's
certificate has been signed by a known CA. The service action would be for the
admin to upload the CA certificate to the BMC.

To upload the CA certificate to the BMC, refer to the "Update LDAP root
certificate" section of this document.

b) openbmctool doesn't support individual LDAP config property update,
   To update a single property user need to recreate the LDAP config with the
   changed values.

### Delete/Erase LDAP Configuration
```
openbmctool.py <connection options> ldap disable

```
NOTE: Make sure that root user is enabled before running the above command
otherwise BMC would not be accessible.

To enable root user, refer to the "To re-enable all local user accounts"
section of this document.

Currently openbmctool doesn't have support for specific user enablement.

### Add privilege mapping

```
openbmctool.py <connection options> ldap privilege-mapper create --groupName=<groupName> --privilege="priv-admin/priv-user"
```

### Delete privilege mapping

```
openbmctool.py <connection options> ldap privilege-mapper delete --groupName=<groupName>
```

### List privilege mapping

```
openbmctool.py <connection options> ldap privilege-mapper list
```

The normal workflow for LDAP configuration would be as shown below

- Configure the DNS server.
- Configure LDAP.
   a) Configure CA certificate if secure LDAP server is being configured.
   b) Create LDAP Configuration with local user.
- Configure user privilege.

NOTE:

a) If a user tries to login with LDAP credentials and has not added the
privilege mapping for the LDAP credentials then the user will get the following
http error code and message.

403, 'LDAP group privilege mapping does not exist'.

Action: Add the privilege (refer to the section "Add privilege mapping")


b) The following message may mean that the user lacks the required privileges
on the BMC:
"Insufficient Privilege"

Action: Add the privilege (refer to the section "Add privilege mapping") with
privilege=priv-admin

c) Once LDAP is set up, openbmctool connection options work with both LDAP
and local users.
