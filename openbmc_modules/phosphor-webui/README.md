# OpenBMC Web User Interface

The OpenBMC WebUI is a Web-based user interface for the OpenBMC
firmware stack. The WebUI uses AngularJS. Features include:

- View system overview data such as model information and serial number
- View and manage event logs
- Inventory data
- Sensor data
- Power On/Off server operations
- Reboot BMC
- SOL console
- Remote KVM
- Virtual media
- Date and time settings
- IPv4 network settings
- Manage and update BMC and Host firmware
- LDAP
- SSL certificates
- Local user management

## Requirements

nodejs (>= 4.2.6)
npm (>= 6.0.1)

**Note** The default installation of your Linux distro may not come with the
required versions above. See the following for more information on updating:

https://docs.npmjs.com/troubleshooting/try-the-latest-stable-version-of-node
https://docs.npmjs.com/troubleshooting/try-the-latest-stable-version-of-npm

## Installation

`npm install`

**Note** This must be run from within the phosphor-webui git repository.

## Running locally

`npm run-script server`

This will start a server instance and begin listening for connections at
`http://localhost:8080`. This development server provides live reloading on
code changes.
NOTE: Browsing to `https://<BMC>` and accepting the self-signed certificate
might be required to prevent your browser from blocking traffic to the BMC.

## Logging in

Enter the BMC Host or BMC IP address, username, and password.
The default username and password are `root`/`0penBmc`.

**Note** that some OpenBMC implementations use [bmcweb](https://github.com/openbmc/bmcweb)
for its backend. For security reasons, bmcweb will need to be recompiled and
loaded onto the target BMC Host before the above redirect command will work. The
option to turn on within bmcweb is `BMCWEB_INSECURE_DISABLE_XSS_PREVENTION`. In
order to test locally, you will also need to disable CSRF by turning on `BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION`.
