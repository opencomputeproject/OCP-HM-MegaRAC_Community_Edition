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
git clone https://github.com/opencomputeproject/MegaRAC-OpenEdition openbmc
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
Root User:
Use root user to build the image for the targetted platform.

Once you know the target, then set it as mentioned below, Currently we are supporting Mitac Aowanda platform:

```
TEMPLATECONF=meta-ami/meta-aowanda/conf  . openbmc-env

```

### 4) Build

```
bitbake obmc-phosphor-image
```

## Supported Features

**Feature List**
* Web-based user interface
* REST interfaces
* D-Bus based interfaces
* Serial over Lan (SOL)
* Remote KVM
* User management
* Power Operations
