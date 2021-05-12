# "Megarac OpenEdition" : AMI's OpenBMC
An improved source tree based on openbmc-LF fully managed and maintained by AMI and contributed to OCP

The AMI's Megarac OpenEdition project is forked from OpenBMC Linux distribution to support BMC functionality in OCP compliant platforms. 
It includes platform specific features and out-of-band remote management.

Project uses same technologies as used by openbmc LF
such as [Yocto](https://www.yoctoproject.org/),
[OpenEmbedded](https://www.openembedded.org/wiki/Main_Page),
[systemd](https://www.freedesktop.org/wiki/Software/systemd/), and
[D-Bus](https://www.freedesktop.org/wiki/Software/dbus/) to allow easy
customization for your server platform.


## Setting up your project

### 1) Prerequisite
- Ubuntu 18.04

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
It will list of all the available target machines in the repo.

```

Once you know the target, then set it as mentioned below, Currently we are supporting tiogapass platform:

```
TEMPLATECONF=meta-ami/meta-tiogapass/conf  . openbmc-env

```

### 4) Build

```
bitbake obmc-phosphor-image
```

## Supported Features

**Feature List**
* IPMI v2.0 and DCMI v1.5
* Sensor monitoring, Event log and FRU
* Web-based user interface
* REST interfaces
* D-Bus based interfaces
* Remote KVM
* User management
* Virtual media
* Fimware Update
* Redfish v1.9
* Lan, KCS and IPMB interfaces
* Certificate management
* LDAP
* Chassis Power control
* Post code
* Thermal management
* Watchdog
* NTP
* I2c, Fan and PWM, ADC, Snoop, GPIO, UART, LPC and PECI

## More documents can be found from

[docs](https://github.com/opencomputeproject/AMI-Tioga-Pass-OpenBMC/tree/master/docs) repository.
