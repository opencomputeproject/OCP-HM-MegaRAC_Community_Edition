# OpenBMC

[![Build Status](https://openpower.xyz/buildStatus/icon?job=openbmc-build)](https://openpower.xyz/job/openbmc-build/)

The OpenBMC project can be described as a Linux distribution for embedded
devices that have a BMC; typically, but not limited to, things like servers,
top of rack switches or RAID appliances. The OpenBMC stack uses technologies
such as [Yocto](https://www.yoctoproject.org/),
[OpenEmbedded](https://www.openembedded.org/wiki/Main_Page),
[systemd](https://www.freedesktop.org/wiki/Software/systemd/), and
[D-Bus](https://www.freedesktop.org/wiki/Software/dbus/) to allow easy
customization for your server platform.


## Setting up your OpenBMC project

### 1) Prerequisite
- Ubuntu 14.04

```
sudo apt-get install -y git build-essential libsdl1.2-dev texinfo gawk chrpath diffstat
```

- Fedora 28

```
sudo dnf install -y git patch diffstat texinfo chrpath SDL-devel bitbake \
    rpcgen perl-Thread-Queue perl-bignum perl-Crypt-OpenSSL-Bignum
sudo dnf groupinstall "C Development Tools and Libraries"
```
### 2) Download the source
```
git clone https://github.com/opencomputeproject/AMI-Tioga-Pass-OpenBMC openbmc
cd openbmc
```

### 3) Target your hardware
Any build requires an environment set up according to your hardware target.
There is a special script in the root of this repository that can be used
to configure the environment as needed. The script is called `setup` and
takes the name of your hardware target as an argument.

The script needs to be sourced while in the top directory of the OpenBMC
repository clone, and, if run without arguments, will display the list
of supported hardware targets, see the following example:

```
$ . setup
Target machine must be specified. Use one of:

centriq2400-rep         nicole                     stardragon4800-rep2
f0b                     olympus                    swift
fp5280g2                olympus-nuvoton            tiogapass
gsj                     on5263m5                   vesnin
hr630                   palmetto                   witherspoon
hr855xg2                qemuarm                    witherspoon-128
lanyang                 quanta-q71l                witherspoon-tacoma
mihawk                  rainier                    yosemitev2
msn                     romulus                    zaius
neptune                 s2600wf
```

Once you know the target (e.g. romulus), source the `setup` script as follows:

```
TEMPLATECONF=meta-ami/meta-tiogapass/conf  . openbmc-env

```

### 4) Build

```
bitbake obmc-phosphor-image
```

Additional details can be found in the [docs](https://github.com/openbmc/docs)
repository.

## Supported Features

**Feature List**
* Host management: Power, Cooling, LEDs, Inventory, Events, Watchdog
* Full IPMI 2.0 Compliance with DCMI
* Code Update Support for multiple BMC/BIOS images
* Web-based user interface
* REST interfaces
* D-Bus based interfaces
* SSH based SOL
* Remote KVM
* Hardware Simulation
* Automated Testing
* User management
* Virtual media

**Features In Progress**
* OpenCompute Redfish Compliance
* Verified Boot

**Features Requested but need help**
* OpenBMC performance monitoring


## Finding out more

Dive deeper into OpenBMC by opening the
[docs](https://github.com/openbmc/docs) repository.
